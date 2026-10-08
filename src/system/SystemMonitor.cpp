#include "SystemMonitor.h"
#include "Logger.h"
#include "../config/AppConfig.h"

#if defined(ARDUINO)
#include <Arduino.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#elif defined(ESP_PLATFORM)
#include "esp_task_wdt.h"
#include "esp_system.h"
#include "esp_timer.h"
#else
#include <chrono>
#endif

uint32_t SystemMonitor::s_bootTimestampMs = 0;
uint32_t SystemMonitor::s_lastOBDHeartbeat = 0;
uint32_t SystemMonitor::s_lastTelemHeartbeat = 0;
uint32_t SystemMonitor::s_lastBLEHeartbeat = 0;

static uint32_t getCurrentMs() {
#if defined(ARDUINO)
    return millis();
#elif defined(ESP_PLATFORM)
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
#else
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
#endif
}

void SystemMonitor::init() {
    s_bootTimestampMs = getCurrentMs();
    s_lastOBDHeartbeat = s_bootTimestampMs;
    s_lastTelemHeartbeat = s_bootTimestampMs;
    s_lastBLEHeartbeat = s_bootTimestampMs;

#if defined(ESP_PLATFORM) || defined(ARDUINO)
    // Initialize Hardware/Task Watchdog (TWDT)
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = WATCHDOG_TIMEOUT_SECONDS * 1000,
        .idle_core_mask = 0,
        .trigger_panic = false
    };
    esp_task_wdt_init(&twdt_config);
    esp_task_wdt_add(NULL); // Subscribe current task
    Logger::logSystem("Task Watchdog initialized (timeout: %d s)", WATCHDOG_TIMEOUT_SECONDS);
#endif
}

void SystemMonitor::feedWatchdog() {
#if defined(ESP_PLATFORM) || defined(ARDUINO)
    esp_task_wdt_reset();
#endif
}

void SystemMonitor::reportHeartbeat(const char* taskName) {
    uint32_t now = getCurrentMs();
    if (!taskName) return;

    if (taskName[0] == 'O' || taskName[0] == 'o') {
        s_lastOBDHeartbeat = now;
    } else if (taskName[0] == 'T' || taskName[0] == 't') {
        s_lastTelemHeartbeat = now;
    } else if (taskName[0] == 'B' || taskName[0] == 'b') {
        s_lastBLEHeartbeat = now;
    }
}

SystemHealth SystemMonitor::getHealth() {
    SystemHealth health = {};
    uint32_t now = getCurrentMs();
    health.uptimeSeconds = (now - s_bootTimestampMs) / 1000;

#if defined(ESP_PLATFORM) || defined(ARDUINO)
    health.freeHeapBytes = esp_get_free_heap_size();
    health.minFreeHeapBytes = esp_get_minimum_free_heap_size();
#else
    health.freeHeapBytes = 180000;
    health.minFreeHeapBytes = 160000;
#endif

    // Consider healthy if heartbeat received within 30 seconds
    const uint32_t maxHeartbeatLag = 30000;
    health.obdTaskHealthy = (now - s_lastOBDHeartbeat) < maxHeartbeatLag;
    health.telemetryTaskHealthy = (now - s_lastTelemHeartbeat) < maxHeartbeatLag;
    health.bleTaskHealthy = (now - s_lastBLEHeartbeat) < maxHeartbeatLag;

    return health;
}

void SystemMonitor::printStatus() {
    SystemHealth h = getHealth();
    Logger::logSystem("Uptime: %us | Free Heap: %u bytes (Min: %u) | Tasks: OBD [%s] Telem [%s] BLE [%s]",
        h.uptimeSeconds,
        h.freeHeapBytes,
        h.minFreeHeapBytes,
        h.obdTaskHealthy ? "OK" : "WARN",
        h.telemetryTaskHealthy ? "OK" : "WARN",
        h.bleTaskHealthy ? "OK" : "WARN"
    );
}
