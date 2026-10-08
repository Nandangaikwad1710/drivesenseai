#define DRIVESENSE_TEST_SUITE 1

#include <iostream>
#include <cassert>
#include <cmath>
#include <cstring>
#include <string>

#include "src/config/HardwareConfig.h"
#include "src/config/AppConfig.h"
#include "src/system/Logger.h"
#include "src/system/ErrorManager.h"
#include "src/system/SystemMonitor.h"
#include "src/obd/SupportedPIDs.h"
#include "src/obd/OBDPID.h"
#include "src/obd/OBDParser.h"
#include "src/telemetry/TelemetryData.h"
#include "src/protocol/JSONSerializer.h"
#include "src/protocol/BinarySerializer.h"
#include "src/telemetry/TelemetryManager.h"
#include "src/ble/BLEManager.h"

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[-] FAILED: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
            return false; \
        } \
    } while (0)

#define RUN_TEST(fn) \
    do { \
        std::cout << "[RUN] " << #fn << "..." << std::flush; \
        if (fn()) { \
            std::cout << " PASSED" << std::endl; \
            passed++; \
        } else { \
            std::cout << " FAILED" << std::endl; \
            failed++; \
        } \
    } while (0)

// =========================================================================
// Test Cases
// =========================================================================

bool test_pid_conversion_formulas() {
    // 1. Vehicle Speed: Speed = A km/h
    float speed = OBDParser::calculateSpeed(75);
    TEST_ASSERT(std::fabs(speed - 75.0f) < 0.001f, "Speed formula failed");

    // 2. Engine RPM: ((A * 256) + B) / 4
    // 0x27 = 39, 0x10 = 16 -> (39 * 256 + 16) / 4 = (9984 + 16) / 4 = 2500.0 RPM
    float rpm = OBDParser::calculateRPM(0x27, 0x10);
    TEST_ASSERT(std::fabs(rpm - 2500.0f) < 0.001f, "RPM formula failed");

    // Idle RPM test: A = 0x0C, B = 0x80 -> (12 * 256 + 128) / 4 = 3200 / 4 = 800 RPM
    float idleRpm = OBDParser::calculateRPM(0x0C, 0x80);
    TEST_ASSERT(std::fabs(idleRpm - 800.0f) < 0.001f, "Idle RPM calculation failed");

    // 3. Coolant Temp: A - 40
    float coolant90 = OBDParser::calculateCoolantTemp(130);
    TEST_ASSERT(std::fabs(coolant90 - 90.0f) < 0.001f, "Coolant 90C failed");
    float coolantZero = OBDParser::calculateCoolantTemp(40);
    TEST_ASSERT(std::fabs(coolantZero - 0.0f) < 0.001f, "Coolant 0C failed");
    float coolantSubZero = OBDParser::calculateCoolantTemp(0);
    TEST_ASSERT(std::fabs(coolantSubZero - (-40.0f)) < 0.001f, "Coolant -40C failed");

    // 4. Engine Load: (A * 100) / 255
    float load100 = OBDParser::calculateEngineLoad(255);
    TEST_ASSERT(std::fabs(load100 - 100.0f) < 0.001f, "Engine Load 100% failed");
    float load50 = OBDParser::calculateEngineLoad(128);
    TEST_ASSERT(std::fabs(load50 - 50.196f) < 0.01f, "Engine Load 50% failed");

    // 5. Throttle Position: (A * 100) / 255
    float throttle0 = OBDParser::calculateThrottlePos(0);
    TEST_ASSERT(std::fabs(throttle0 - 0.0f) < 0.001f, "Throttle 0% failed");
    float throttle100 = OBDParser::calculateThrottlePos(255);
    TEST_ASSERT(std::fabs(throttle100 - 100.0f) < 0.001f, "Throttle 100% failed");

    // 6. Intake Air Temperature: A - 40
    float iat = OBDParser::calculateIntakeTemp(68);
    TEST_ASSERT(std::fabs(iat - 28.0f) < 0.001f, "IAT calculation failed");

    // 7. Mass Air Flow: ((A * 256) + B) / 100
    // A = 0x03, B = 0xE8 -> 1000 / 100 = 10.00 g/s
    float maf = OBDParser::calculateMAF(0x03, 0xE8);
    TEST_ASSERT(std::fabs(maf - 10.00f) < 0.001f, "MAF calculation failed");

    // 8. Fuel Level: (A * 100) / 255
    float fuelHalf = OBDParser::calculateFuelLevel(128);
    TEST_ASSERT(std::fabs(fuelHalf - 50.196f) < 0.01f, "Fuel level half calculation failed");

    return true;
}

bool test_obd_parser_frame_validation() {
    // Valid Positive Response for RPM (PID 0x0C):
    // Byte 0: 0x04 (len = 4 bytes: 0x41, 0x0C, A, B)
    // Byte 1: 0x41 (positive response mode)
    // Byte 2: 0x0C (PID)
    // Byte 3: 0x27 (A)
    // Byte 4: 0x10 (B)
    uint8_t rpmFrame[8] = { 0x04, 0x41, 0x0C, 0x27, 0x10, 0x55, 0x55, 0x55 };
    OBDParseResult res = OBDParser::parseFrame(rpmFrame, 8, OBD::PID_ENGINE_RPM);
    TEST_ASSERT(res.isValid, "Valid RPM frame reported invalid");
    TEST_ASSERT(!res.isNegativeResponse, "Reported negative on valid positive frame");
    TEST_ASSERT(res.pid == OBD::PID_ENGINE_RPM, "PID mismatch");
    TEST_ASSERT(std::fabs(res.convertedValue - 2500.0f) < 0.001f, "Converted value mismatch");

    // Valid Positive Response for Speed (PID 0x0D):
    uint8_t speedFrame[8] = { 0x03, 0x41, 0x0D, 0x50, 0x55, 0x55, 0x55, 0x55 }; // 0x50 = 80 km/h
    OBDParseResult speedRes = OBDParser::parseFrame(speedFrame, 8, OBD::PID_VEHICLE_SPEED);
    TEST_ASSERT(speedRes.isValid, "Valid Speed frame reported invalid");
    TEST_ASSERT(std::fabs(speedRes.convertedValue - 80.0f) < 0.001f, "Speed value mismatch");

    // Mismatched PID in response
    OBDParseResult mismatchRes = OBDParser::parseFrame(speedFrame, 8, OBD::PID_ENGINE_RPM);
    TEST_ASSERT(!mismatchRes.isValid, "Mismatched PID should not be valid");

    // Negative Response (0x7F)
    // Byte 0: 0x03
    // Byte 1: 0x7F (Negative response)
    // Byte 2: 0x01 (Requested Mode)
    // Byte 3: 0x11 (Service Not Supported)
    uint8_t negFrame[8] = { 0x03, 0x7F, 0x01, 0x11, 0x55, 0x55, 0x55, 0x55 };
    OBDParseResult negRes = OBDParser::parseFrame(negFrame, 8, OBD::PID_ENGINE_RPM);
    TEST_ASSERT(negRes.isNegativeResponse, "Negative response not detected");
    TEST_ASSERT(negRes.nrc == 0x11, "NRC code incorrect");

    // Frame builder test
    uint8_t reqBuffer[8] = {0};
    OBDParser::buildRequestFrame(0x01, 0x0C, reqBuffer);
    TEST_ASSERT(reqBuffer[0] == 0x02, "Request byte 0 length wrong");
    TEST_ASSERT(reqBuffer[1] == 0x01, "Request mode wrong");
    TEST_ASSERT(reqBuffer[2] == 0x0C, "Request PID wrong");
    TEST_ASSERT(reqBuffer[3] == 0x55, "Request padding wrong");

    return true;
}

bool test_supported_pid_bitmask_decoding() {
    // In SAE J1979:
    // PID 0x01 is MSB of Byte A (bit 31)
    // PID 0x04 is bit 28
    // PID 0x05 is bit 27
    // PID 0x0C is bit 20
    // PID 0x0D is bit 19
    // PID 0x20 is LSB of Byte D (bit 0)
    
    // Mask with PIDs: 0x04 (Load), 0x05 (Coolant), 0x0C (RPM), 0x0D (Speed), 0x20 (Next range supported)
    // Bit 28: 1 << 28 = 0x10000000
    // Bit 27: 1 << 27 = 0x08000000
    // Bit 20: 1 << 20 = 0x00100000
    // Bit 19: 1 << 19 = 0x00080000
    // Bit 0:  1 << 0  = 0x00000001
    uint32_t mask = (1UL << (32 - 4))  | // PID 0x04
                    (1UL << (32 - 5))  | // PID 0x05
                    (1UL << (32 - 12)) | // PID 0x0C
                    (1UL << (32 - 13)) | // PID 0x0D
                    (1UL << (32 - 32));  // PID 0x20

    TEST_ASSERT(OBDParser::isPidSupportedInMask(0x00, OBD::PID_ENGINE_LOAD, mask), "PID 0x04 should be supported");
    TEST_ASSERT(OBDParser::isPidSupportedInMask(0x00, OBD::PID_COOLANT_TEMP, mask), "PID 0x05 should be supported");
    TEST_ASSERT(OBDParser::isPidSupportedInMask(0x00, OBD::PID_ENGINE_RPM, mask), "PID 0x0C should be supported");
    TEST_ASSERT(OBDParser::isPidSupportedInMask(0x00, OBD::PID_VEHICLE_SPEED, mask), "PID 0x0D should be supported");
    TEST_ASSERT(OBDParser::isPidSupportedInMask(0x00, OBD::PID_SUPPORTED_21_40, mask), "PID 0x20 should be supported");

    // Check unsupported PIDs in that mask
    TEST_ASSERT(!OBDParser::isPidSupportedInMask(0x00, 0x01, mask), "PID 0x01 should NOT be supported");
    TEST_ASSERT(!OBDParser::isPidSupportedInMask(0x00, OBD::PID_THROTTLE_POS, mask), "PID 0x11 should NOT be supported");

    return true;
}

bool test_json_serializer_and_missing_data() {
    JSONSerializer serializer;
    char buffer[512];

    // Case 1: All sensors available
    TelemetryData fullData;
    fullData.timestamp = 1725700000;
    fullData.speed = 62.4f; fullData.speedAvailable = true;
    fullData.rpm = 2450.0f; fullData.rpmAvailable = true;
    fullData.engineLoad = 48.2f; fullData.engineLoadAvailable = true;
    fullData.throttlePosition = 32.5f; fullData.throttleAvailable = true;
    fullData.coolantTemperature = 88.0f; fullData.coolantTemperatureAvailable = true;
    fullData.intakeAirTemperature = 31.0f; fullData.intakeAirTemperatureAvailable = true;
    fullData.massAirFlow = 8.4f; fullData.mafAvailable = true;
    fullData.fuelLevel = 62.0f; fullData.fuelLevelAvailable = true;

    size_t len1 = serializer.serialize(fullData, buffer, sizeof(buffer));
    TEST_ASSERT(len1 > 0, "Serialization failed for full data");
    std::string json1(buffer);

    TEST_ASSERT(json1.find("\"speed\":62.4") != std::string::npos, "Speed JSON missing");
    TEST_ASSERT(json1.find("\"rpm\":2450") != std::string::npos, "RPM JSON missing");
    TEST_ASSERT(json1.find("\"fuel_level\":62.0") != std::string::npos, "Fuel level JSON missing");

    // Case 2: Missing Data (fuel_level and mass_air_flow unsupported)
    TelemetryData partialData;
    partialData.timestamp = 1725700000;
    partialData.speed = 62.4f; partialData.speedAvailable = true;
    partialData.rpm = 2450.0f; partialData.rpmAvailable = true;
    partialData.fuelLevelAvailable = false; // Missing!
    partialData.mafAvailable = false;       // Missing!
    partialData.throttleAvailable = true;
    partialData.throttlePosition = 25.0f;

    size_t len2 = serializer.serialize(partialData, buffer, sizeof(buffer));
    TEST_ASSERT(len2 > 0, "Serialization failed for partial data");
    std::string json2(buffer);

    TEST_ASSERT(json2.find("\"fuel_level\":null") != std::string::npos, "fuel_level must be null when unavailable");
    TEST_ASSERT(json2.find("\"mass_air_flow\":null") != std::string::npos, "mass_air_flow must be null when unavailable");
    TEST_ASSERT(json2.find("\"speed\":62.4") != std::string::npos, "speed must be present");

    return true;
}

bool test_binary_serializer() {
    BinarySerializer binSerializer;
    char outBuf[128];

    TelemetryData telem;
    telem.timestamp = 10000;
    telem.speed = 72.5f; telem.speedAvailable = true;
    telem.rpm = 2000.0f; telem.rpmAvailable = true;

    size_t bytes = binSerializer.serialize(telem, outBuf, sizeof(outBuf));
    TEST_ASSERT(bytes == sizeof(BinaryTelemetryPacket), "Binary packet size mismatch");

    const BinaryTelemetryPacket* pkt = reinterpret_cast<const BinaryTelemetryPacket*>(outBuf);
    TEST_ASSERT(pkt->header[0] == 0xAA && pkt->header[1] == 0x55, "Magic headers mismatch");
    TEST_ASSERT((pkt->availableFlags & (1 << 0)) != 0, "Speed flag not set");
    TEST_ASSERT(pkt->speedX10 == 725, "Speed scaled value mismatch");
    TEST_ASSERT(pkt->rpmX4 == 8000, "RPM scaled value mismatch");

    return true;
}

bool test_telemetry_buffer_overflow() {
    TelemetryBuffer buffer;
    TEST_ASSERT(buffer.isEmpty(), "New buffer should be empty");
    TEST_ASSERT(buffer.size() == 0, "New buffer count should be 0");

    TelemetryData item;
    item.speedAvailable = true;

    // Fill buffer up to capacity
    for (size_t i = 0; i < TELEMETRY_BUFFER_CAPACITY; ++i) {
        item.speed = (float)i;
        bool storedWithoutDrop = buffer.push(item);
        TEST_ASSERT(storedWithoutDrop, "Item should be stored without drop when not full");
    }

    TEST_ASSERT(buffer.size() == TELEMETRY_BUFFER_CAPACITY, "Buffer should be full");
    TEST_ASSERT(buffer.getDroppedCount() == 0, "Dropped count should be 0 before overflow");

    // Push 5 more items to trigger oldest-discard policy
    for (size_t i = 0; i < 5; ++i) {
        item.speed = 100.0f + (float)i;
        bool storedWithoutDrop = buffer.push(item);
        TEST_ASSERT(!storedWithoutDrop, "Pushing to full buffer should drop oldest");
    }

    TEST_ASSERT(buffer.size() == TELEMETRY_BUFFER_CAPACITY, "Buffer size must not exceed capacity");
    TEST_ASSERT(buffer.getDroppedCount() == 5, "Dropped count should be exactly 5");

    // Check that oldest items popped are items 5, 6, 7... (0..4 were discarded)
    TelemetryData popped;
    bool popOk = buffer.pop(popped);
    TEST_ASSERT(popOk, "Pop should succeed");
    TEST_ASSERT(std::fabs(popped.speed - 5.0f) < 0.001f, "First popped item should be item 5");

    // Peek latest item
    TelemetryData latest;
    TEST_ASSERT(buffer.peekLatest(latest), "Peek latest should succeed");
    TEST_ASSERT(std::fabs(latest.speed - 104.0f) < 0.001f, "Latest item should be 104");

    return true;
}

bool test_simulation_driving_cycle() {
    TelemetryManager telemMgr;
    telemMgr.init(nullptr, true); // Initialized with simulationMode = true

    TEST_ASSERT(telemMgr.isSimulation(), "Should be in simulation mode");

    telemMgr.startStreaming();
    TEST_ASSERT(telemMgr.isStreaming(), "Should be streaming");

    // Step simulation through multiple time points
    telemMgr.update(0);
    TelemetryData d1 = telemMgr.getCurrentData();
    TEST_ASSERT(d1.speedAvailable && d1.rpmAvailable, "Simulation should provide speed & rpm");
    TEST_ASSERT(d1.rpm >= 700.0f && d1.rpm <= 900.0f, "Idle RPM should be around 800");
    TEST_ASSERT(d1.speed == 0.0f, "Idle speed should be 0");

    // Transition at 5s to Acceleration, then test active acceleration at 8s
    telemMgr.update(5000); // Triggers transition to Accelerating
    telemMgr.update(8000); // 3 seconds into Accelerating phase
    TelemetryData d2 = telemMgr.getCurrentData();
    TEST_ASSERT(d2.speed > 0.0f, "Vehicle should be moving in acceleration phase");
    TEST_ASSERT(d2.rpm > 850.0f, "RPM should be higher in acceleration phase");
    TEST_ASSERT(d2.throttlePosition > 20.0f, "Throttle should be pressed");

    // Advance to Cruising phase (transition at 12s, tested at 15s)
    telemMgr.update(12001); // Triggers transition to Cruising
    telemMgr.update(15000); // 3 seconds into Cruising phase
    TelemetryData d3 = telemMgr.getCurrentData();
    TEST_ASSERT(d3.speed >= 50.0f, "Cruising speed should be steady above 50 km/h");

    // Advance to Deceleration phase (transition at 20s, tested at 22s)
    telemMgr.update(20001); // Triggers transition to Decelerating
    telemMgr.update(22000); // 2 seconds into Decelerating phase
    TelemetryData d4 = telemMgr.getCurrentData();
    TEST_ASSERT(d4.throttlePosition == 0.0f, "Throttle should be 0 during deceleration");

    telemMgr.stopStreaming();
    TEST_ASSERT(!telemMgr.isStreaming(), "Streaming should be stopped");

    return true;
}

static CommandType s_lastCmdReceived = CommandType::UNKNOWN;
static void testCmdCallback(CommandType cmd, const char* raw, void* ctx) {
    (void)raw;
    (void)ctx;
    s_lastCmdReceived = cmd;
}

bool test_ble_command_parsing() {
    BLEManager ble;
    ble.setCommandCallback(testCmdCallback, nullptr);

    // Test START_TELEMETRY
    s_lastCmdReceived = CommandType::UNKNOWN;
    ble.processIncomingCommand("START_TELEMETRY");
    TEST_ASSERT(s_lastCmdReceived == CommandType::START_TELEMETRY, "START_TELEMETRY command parsing failed");

    // Test JSON formatted command
    s_lastCmdReceived = CommandType::UNKNOWN;
    ble.processIncomingCommand("{\"command\":\"STOP_TELEMETRY\"}");
    TEST_ASSERT(s_lastCmdReceived == CommandType::STOP_TELEMETRY, "STOP_TELEMETRY JSON parsing failed");

    // Test GET_DEVICE_INFO
    s_lastCmdReceived = CommandType::UNKNOWN;
    ble.processIncomingCommand("GET_DEVICE_INFO");
    TEST_ASSERT(s_lastCmdReceived == CommandType::GET_DEVICE_INFO, "GET_DEVICE_INFO parsing failed");

    // Test GET_SUPPORTED_PIDS
    s_lastCmdReceived = CommandType::UNKNOWN;
    ble.processIncomingCommand("GET_SUPPORTED_PIDS");
    TEST_ASSERT(s_lastCmdReceived == CommandType::GET_SUPPORTED_PIDS, "GET_SUPPORTED_PIDS parsing failed");

    // Test GET_CONNECTION_STATUS
    s_lastCmdReceived = CommandType::UNKNOWN;
    ble.processIncomingCommand("GET_CONNECTION_STATUS");
    TEST_ASSERT(s_lastCmdReceived == CommandType::GET_CONNECTION_STATUS, "GET_CONNECTION_STATUS parsing failed");

    // Test SET_SIMULATION_MODE
    s_lastCmdReceived = CommandType::UNKNOWN;
    ble.processIncomingCommand("SET_SIMULATION_MODE false");
    TEST_ASSERT(s_lastCmdReceived == CommandType::SET_SIMULATION_MODE, "SET_SIMULATION_MODE parsing failed");

    return true;
}

bool test_error_manager_and_system_health() {
    ErrorManager::init();
    TEST_ASSERT(ErrorManager::getStats().obdTimeouts == 0, "Timeouts should start at 0");

    ErrorManager::recordError(ErrorCode::OBD_TIMEOUT);
    ErrorManager::recordError(ErrorCode::OBD_TIMEOUT);
    ErrorManager::recordError(ErrorCode::CAN_TRANSMIT_FAILED);
    ErrorManager::recordError(ErrorCode::BLE_DISCONNECTED);

    const ErrorStats& stats = ErrorManager::getStats();
    TEST_ASSERT(stats.obdTimeouts == 2, "Should record 2 timeouts");
    TEST_ASSERT(stats.canTransmitErrors == 1, "Should record 1 CAN error");
    TEST_ASSERT(stats.bleDisconnects == 1, "Should record 1 BLE disconnect");
    TEST_ASSERT(stats.lastError == ErrorCode::BLE_DISCONNECTED, "Last error mismatch");

    SystemMonitor::init();
    SystemHealth health = SystemMonitor::getHealth();
    TEST_ASSERT(health.freeHeapBytes > 0, "Free heap should be positive");
    TEST_ASSERT(health.obdTaskHealthy, "OBD task should be healthy after init");

    return true;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   DriveSense OBD Unit Test Suite       " << std::endl;
    std::cout << "========================================" << std::endl;

    int passed = 0;
    int failed = 0;

    RUN_TEST(test_pid_conversion_formulas);
    RUN_TEST(test_obd_parser_frame_validation);
    RUN_TEST(test_supported_pid_bitmask_decoding);
    RUN_TEST(test_json_serializer_and_missing_data);
    RUN_TEST(test_binary_serializer);
    RUN_TEST(test_telemetry_buffer_overflow);
    RUN_TEST(test_simulation_driving_cycle);
    RUN_TEST(test_ble_command_parsing);
    RUN_TEST(test_error_manager_and_system_health);

    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Test Summary: " << passed << " Passed, " << failed << " Failed." << std::endl;
    std::cout << "========================================" << std::endl;

    return failed == 0 ? 0 : 1;
}
