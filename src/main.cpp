#include <stdio.h>
#include <string.h>

#include "config/HardwareConfig.h"
#include "config/AppConfig.h"
#include "system/Logger.h"
#include "system/ErrorManager.h"
#include "system/SystemMonitor.h"
#include "can/CANManager.h"
#include "obd/OBDManager.h"
#include "telemetry/TelemetryManager.h"
#include "protocol/JSONSerializer.h"
#include "ble/BLEManager.h"

#if defined(ARDUINO) && !defined(DRIVESENSE_TEST_SUITE)
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#endif

// ==========================================
// Global Component Instances
// ==========================================
static CANManager s_canManager;
static OBDManager s_obdManager(&s_canManager);
static TelemetryManager s_telemetryManager(&s_obdManager);
static JSONSerializer s_jsonSerializer;
static BLEManager s_bleManager;

#if defined(ARDUINO) && !defined(DRIVESENSE_TEST_SUITE)
// FreeRTOS Queues
static QueueHandle_t s_telemetryQueue = nullptr;
static QueueHandle_t s_bleNotifyQueue = nullptr;
static QueueHandle_t s_commandQueue   = nullptr;

// Struct for serialized BLE notification queue
struct BLENotifyMessage {
    char payload[384];
    size_t length;
};
#endif

// ==========================================
// Forward Declarations
// ==========================================
static void onMobileCommandReceived(CommandType cmd, const char* rawPayload, void* context);
static void handleCommandExecution(CommandType cmd, const char* rawPayload);

// ==========================================
// FreeRTOS Task Implementations
// ==========================================

#if defined(ARDUINO) && !defined(DRIVESENSE_TEST_SUITE)

/**
 * @brief Task 1: CAN and OBD-II Communication Task
 * Priority: 3 (Higher priority to satisfy strict CAN bus timing)
 */
void obdTask(void* parameter) {
    Logger::logOBD("OBD-II Polling Task started on Core %d", xPortGetCoreID());

    // Initialize CAN controller
    bool simMode = SIMULATION_MODE;
    bool canOk = s_canManager.init(CANSpeed::CAN_500KBPS, CAN_OSC_CLOCK_MHZ, simMode);
    if (!canOk) {
        Logger::warn("OBD", "Hardware CAN init failed. Falling back to Simulation Mode.");
        simMode = true;
        s_canManager.setSimulation(true);
    }

    s_obdManager.init(&s_canManager, simMode);
    s_telemetryManager.init(&s_obdManager, simMode);

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(50); // 20Hz loop

    while (true) {
        SystemMonitor::reportHeartbeat("OBD");

        uint32_t nowMs = millis();
        s_telemetryManager.update(nowMs);

        // If telemetry has updated data, queue it for processing
        if (s_telemetryManager.isStreaming()) {
            TelemetryData latest = s_telemetryManager.getCurrentData();
            if (s_telemetryQueue != nullptr) {
                if (xQueueSend(s_telemetryQueue, &latest, 0) != pdPASS) {
                    ErrorManager::recordError(ErrorCode::QUEUE_OVERFLOW);
                }
            }
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

/**
 * @brief Task 2: Telemetry Data Processing & Formatting Task
 * Priority: 2
 */
void telemetryProcessingTask(void* parameter) {
    Logger::logTelemetry("Telemetry Processing Task started on Core %d", xPortGetCoreID());

    TelemetryData incomingData;
    BLENotifyMessage notifyMsg;

    while (true) {
        SystemMonitor::reportHeartbeat("Telem");

        if (xQueueReceive(s_telemetryQueue, &incomingData, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Serialize to JSON
            size_t len = s_jsonSerializer.serialize(incomingData, notifyMsg.payload, sizeof(notifyMsg.payload));
            if (len > 0) {
                notifyMsg.length = len;

                // Push to BLE notification queue (drop oldest if full)
                if (s_bleNotifyQueue != nullptr) {
                    if (xQueueSend(s_bleNotifyQueue, &notifyMsg, 0) != pdPASS) {
                        // Discard oldest to preserve fresh real-time data
                        BLENotifyMessage discarded;
                        xQueueReceive(s_bleNotifyQueue, &discarded, 0);
                        xQueueSend(s_bleNotifyQueue, &notifyMsg, 0);
                    }
                }
            }
        }
    }
}

/**
 * @brief Task 3: Bluetooth Low Energy Communication Task
 * Priority: 2
 */
void bleTask(void* parameter) {
    Logger::logBLE("BLE Task started on Core %d", xPortGetCoreID());

    s_bleManager.init(DEVICE_NAME);
    s_bleManager.setCommandCallback(onMobileCommandReceived, nullptr);

    BLENotifyMessage notifyMsg;
    CommandPayload cmdPayload;

    while (true) {
        SystemMonitor::reportHeartbeat("BLE");

        // 1. Process outgoing telemetry notifications
        if (s_bleNotifyQueue != nullptr) {
            if (xQueueReceive(s_bleNotifyQueue, &notifyMsg, pdMS_TO_TICKS(20)) == pdTRUE) {
                if (s_bleManager.isConnected()) {
                    s_bleManager.sendTelemetryNotification(notifyMsg.payload, notifyMsg.length);
                    
                    // Toggle status LED on transmission
                    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
                }
            }
        }

        // 2. Process incoming mobile application commands
        if (s_commandQueue != nullptr) {
            if (xQueueReceive(s_commandQueue, &cmdPayload, 0) == pdTRUE) {
                handleCommandExecution(cmdPayload.type, nullptr);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief Task 4: System Monitor & Watchdog Supervisor Task
 * Priority: 1 (Lowest priority)
 */
void systemMonitorTask(void* parameter) {
    Logger::logBoot("System Monitor Task started");

    while (true) {
        SystemMonitor::feedWatchdog();
        SystemMonitor::printStatus();

        vTaskDelay(pdMS_TO_TICKS(SYSTEM_HEALTH_LOG_INTERVAL_MS));
    }
}

#endif // ARDUINO

// ==========================================
// Mobile App Command Handlers
// ==========================================

static void onMobileCommandReceived(CommandType cmd, const char* rawPayload, void* context) {
    (void)context;
    Logger::logBLE("Dispatching Command: %d (%s)", (int)cmd, rawPayload ? rawPayload : "");

#if defined(ARDUINO) && !defined(DRIVESENSE_TEST_SUITE)
    CommandPayload payload = { cmd, false, 0 };
    if (s_commandQueue != nullptr) {
        xQueueSend(s_commandQueue, &payload, 0);
    }
#else
    handleCommandExecution(cmd, rawPayload);
#endif
}

static void handleCommandExecution(CommandType cmd, const char* rawPayload) {
    char responseBuf[384];

    switch (cmd) {
        case CommandType::START_TELEMETRY:
            s_telemetryManager.startStreaming();
            snprintf(responseBuf, sizeof(responseBuf),
                     "{\"status\":\"TELEMETRY_STARTED\",\"rate_ms\":%d}",
                     TELEMETRY_STREAM_INTERVAL_MS);
            s_bleManager.sendStatusNotification(responseBuf, strlen(responseBuf));
            break;

        case CommandType::STOP_TELEMETRY:
            s_telemetryManager.stopStreaming();
            snprintf(responseBuf, sizeof(responseBuf), "{\"status\":\"TELEMETRY_STOPPED\"}");
            s_bleManager.sendStatusNotification(responseBuf, strlen(responseBuf));
            break;

        case CommandType::GET_DEVICE_INFO:
            snprintf(responseBuf, sizeof(responseBuf),
                     "{\"device_name\":\"%s\","
                     "\"firmware_version\":\"%s\","
                     "\"hardware_version\":\"%s\","
                     "\"bluetooth_status\":\"%s\","
                     "\"obd_status\":\"%s\"}",
                     DEVICE_NAME,
                     FIRMWARE_VERSION,
                     HARDWARE_VERSION,
                     s_bleManager.isConnected() ? "CONNECTED" : "DISCONNECTED",
                     s_obdManager.getStatus() == OBDStatus::READY ? "CONNECTED" : "NO_RESPONSE");
            s_bleManager.sendStatusNotification(responseBuf, strlen(responseBuf));
            break;

        case CommandType::GET_SUPPORTED_PIDS:
            snprintf(responseBuf, sizeof(responseBuf),
                     "{\"supported_mask_01_20\":\"0x%08X\","
                     "\"supported_mask_21_40\":\"0x%08X\"}",
                     s_obdManager.getSupportedMask0120(),
                     s_obdManager.getSupportedMask2140());
            s_bleManager.sendStatusNotification(responseBuf, strlen(responseBuf));
            break;

        case CommandType::GET_CONNECTION_STATUS:
            snprintf(responseBuf, sizeof(responseBuf),
                     "{\"bluetooth\":\"%s\","
                     "\"obd\":\"%s\","
                     "\"telemetry\":\"%s\","
                     "\"simulation\":%s}",
                     s_bleManager.isConnected() ? "CONNECTED" : "DISCONNECTED",
                     s_obdManager.getStatus() == OBDStatus::READY ? "CONNECTED" : "INITIALIZING",
                     s_telemetryManager.isStreaming() ? "ACTIVE" : "STOPPED",
                     s_telemetryManager.isSimulation() ? "true" : "false");
            s_bleManager.sendStatusNotification(responseBuf, strlen(responseBuf));
            break;

        case CommandType::SET_SIMULATION_MODE: {
            bool sim = true;
            if (rawPayload && strstr(rawPayload, "false")) {
                sim = false;
            }
            s_telemetryManager.setSimulation(sim);
            snprintf(responseBuf, sizeof(responseBuf), "{\"simulation_mode\":%s}", sim ? "true" : "false");
            s_bleManager.sendStatusNotification(responseBuf, strlen(responseBuf));
            break;
        }

        default:
            snprintf(responseBuf, sizeof(responseBuf), "{\"error\":\"UNKNOWN_COMMAND\"}");
            s_bleManager.sendStatusNotification(responseBuf, strlen(responseBuf));
            break;
    }
}

// ==========================================
// Arduino Setup and Main Loop
// ==========================================

#if defined(ARDUINO) && !defined(DRIVESENSE_TEST_SUITE)

void setup() {
    // 1. Initialize Serial Logging
    Serial.begin(SERIAL_BAUD_RATE);
    while (!Serial && millis() < 1500); // Short grace period for USB CDC
    Logger::init(LOG_LEVEL_DEBUG);

    Logger::logBoot("========================================");
    Logger::logBoot("   DriveSense OBD Firmware v%s", FIRMWARE_VERSION);
    Logger::logBoot("   Automotive IoT Telemetry Interface   ");
    Logger::logBoot("========================================");

    // 2. Configure Hardware GPIOs
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);

    // 3. Initialize Error and System Monitors
    ErrorManager::init();
    SystemMonitor::init();

    // 4. Create FreeRTOS Queues
    s_telemetryQueue = xQueueCreate(TELEMETRY_QUEUE_SIZE, sizeof(TelemetryData));
    s_bleNotifyQueue = xQueueCreate(BLE_NOTIFY_QUEUE_SIZE, sizeof(BLENotifyMessage));
    s_commandQueue   = xQueueCreate(BLE_CMD_QUEUE_SIZE, sizeof(CommandPayload));

    if (!s_telemetryQueue || !s_bleNotifyQueue || !s_commandQueue) {
        Logger::error("BOOT", "Failed to allocate FreeRTOS Queues!");
        while (1) { delay(1000); }
    }

    // 5. Spawn FreeRTOS Multitasking Architecture
    // Pin tasks to Core 0 (Networking/BLE/System) and Core 1 (Automotive Real-time CAN/OBD)
    xTaskCreatePinnedToCore(
        obdTask,
        "OBD_Task",
        4096,
        nullptr,
        3,             // High priority
        nullptr,
        1              // Core 1: Automotive CAN/OBD dedicated
    );

    xTaskCreatePinnedToCore(
        telemetryProcessingTask,
        "Telem_Task",
        4096,
        nullptr,
        2,             // Medium priority
        nullptr,
        1              // Core 1
    );

    xTaskCreatePinnedToCore(
        bleTask,
        "BLE_Task",
        4096,
        nullptr,
        2,             // Medium priority
        nullptr,
        0              // Core 0: Radio/BLE stack
    );

    xTaskCreatePinnedToCore(
        systemMonitorTask,
        "Sys_Monitor",
        2048,
        nullptr,
        1,             // Low priority background supervisor
        nullptr,
        0              // Core 0
    );

    Logger::logBoot("All FreeRTOS tasks successfully pinned and launched.");
}

void loop() {
    // FreeRTOS scheduler handles all tasks; idle loop in low-power delay
    vTaskDelay(pdMS_TO_TICKS(1000));
}

#endif // ARDUINO
