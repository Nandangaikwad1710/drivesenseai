#include "CANManager.h"
#include "../system/Logger.h"
#include "../system/ErrorManager.h"
#include "../config/HardwareConfig.h"
#include "../config/AppConfig.h"

#if defined(ARDUINO) && !defined(DRIVESENSE_TEST_SUITE)
#include <Arduino.h>
#include <SPI.h>
#include <mcp_can.h>
#else
#include <chrono>
#include <thread>
#define millis() ((uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count())
#define delay(ms) std::this_thread::sleep_for(std::chrono::milliseconds(ms))
#endif

CANManager::CANManager()
    : m_status(CANStatus::UNINITIALIZED),
      m_speed(CANSpeed::CAN_500KBPS),
      m_clockMHz(CAN_OSC_CLOCK_MHZ),
      m_isSimulation(false),
      m_driver(nullptr) {}

CANManager::~CANManager() {
#if defined(ARDUINO) && !defined(DRIVESENSE_TEST_SUITE)
    if (m_driver) {
        delete static_cast<MCP_CAN*>(m_driver);
        m_driver = nullptr;
    }
#endif
}

bool CANManager::init(CANSpeed speed, uint8_t clockMHz, bool forceSimulation) {
    m_speed = speed;
    m_clockMHz = clockMHz;
    m_isSimulation = forceSimulation;

    if (m_isSimulation) {
        m_status = CANStatus::SIMULATED;
        Logger::logCAN("Operating in SIMULATED CAN mode (Hardware bypassed)");
        return true;
    }

    Logger::logCAN("Initializing MCP2515 CAN controller (CS: %d, INT: %d, Clock: %d MHz)...",
                   CAN_CS_PIN, CAN_INT_PIN, clockMHz);

#if defined(ARDUINO) && !defined(DRIVESENSE_TEST_SUITE)
    // Configure SPI pins
    SPI.begin(CAN_SCK_PIN, CAN_MISO_PIN, CAN_MOSI_PIN, CAN_CS_PIN);
    pinMode(CAN_INT_PIN, INPUT_PULLUP);

    if (!m_driver) {
        m_driver = new MCP_CAN(CAN_CS_PIN);
    }
    MCP_CAN* can = static_cast<MCP_CAN*>(m_driver);

    // Map bitrate
    byte mcpSpeed = CAN_500KBPS;
    if (speed == CANSpeed::CAN_250KBPS) mcpSpeed = CAN_250KBPS;
    else if (speed == CANSpeed::CAN_1000KBPS) mcpSpeed = CAN_1000KBPS;

    // Map oscillator clock
    byte mcpClock = (clockMHz == 16) ? MCP_16MHZ : MCP_8MHZ;

    // Initialize MCP2515 in NORMAL mode
    byte initResult = can->begin(MCP_STDEXT, mcpSpeed, mcpClock);
    if (initResult == CAN_OK) {
        can->setMode(MCP_NORMAL);
        m_status = CANStatus::READY;
        Logger::logCAN("MCP2515 Initialized Successfully @ %s",
                       (speed == CANSpeed::CAN_500KBPS ? "500 kbps" : "250 kbps"));
        return true;
    } else {
        Logger::error("CAN", "MCP2515 Initialization Failed (Code: %d). Check SPI wiring.", initResult);
        ErrorManager::recordError(ErrorCode::CAN_INIT_FAILED);
        m_status = CANStatus::ERROR_HARDWARE;
        return false;
    }
#else
    m_status = CANStatus::SIMULATED;
    m_isSimulation = true;
    Logger::logCAN("Simulated CAN Controller Initialized (Native build)");
    return true;
#endif
}

bool CANManager::sendFrame(const CANFrame& frame) {
    if (m_isSimulation) {
        return true;
    }

    // SAFETY CHECK: Automotive Read-Only Enforcement
    // Strictly forbid sending non-diagnostic frames or arbitrary CAN commands
    if (OBD_READ_ONLY_MODE) {
        bool isStandardDiagnostic = (frame.id == OBD_CAN_BROADCAST_ID) || 
                                    (frame.id >= 0x7E0 && frame.id <= 0x7E7) ||
                                    (frame.id == 0x18DB33F1);
        if (!isStandardDiagnostic) {
            Logger::error("CAN", "SAFETY VIOLATION: Blocked non-diagnostic CAN frame ID 0x%X", frame.id);
            return false;
        }
    }

#if defined(ARDUINO) && !defined(DRIVESENSE_TEST_SUITE)
    if (m_status != CANStatus::READY || !m_driver) {
        return false;
    }

    MCP_CAN* can = static_cast<MCP_CAN*>(m_driver);
    byte sndStat = can->sendMsgBuf(frame.id, frame.isExtended ? 1 : 0, frame.dlc, const_cast<uint8_t*>(frame.data));
    if (sndStat == CAN_OK) {
        return true;
    } else {
        ErrorManager::recordError(ErrorCode::CAN_TRANSMIT_FAILED);
        return false;
    }
#else
    return true;
#endif
}

bool CANManager::checkReceive() {
    if (m_isSimulation) return false;

#if defined(ARDUINO) && !defined(DRIVESENSE_TEST_SUITE)
    if (m_status != CANStatus::READY || !m_driver) return false;
    MCP_CAN* can = static_cast<MCP_CAN*>(m_driver);
    return (can->checkReceive() == CAN_MSGAVAIL);
#else
    return false;
#endif
}

bool CANManager::receiveFrame(CANFrame& frame, uint32_t timeoutMs) {
    if (m_isSimulation) {
        return false;
    }

#if defined(ARDUINO) && !defined(DRIVESENSE_TEST_SUITE)
    if (m_status != CANStatus::READY || !m_driver) return false;
    MCP_CAN* can = static_cast<MCP_CAN*>(m_driver);

    uint32_t start = millis();
    while ((millis() - start) < timeoutMs) {
        if (can->checkReceive() == CAN_MSGAVAIL) {
            unsigned long rxId = 0;
            byte ext = 0;
            byte len = 0;
            byte rxBuf[8] = {0};

            byte stat = can->readMsgBuf(&rxId, &ext, &len, rxBuf);
            if (stat == CAN_OK) {
                frame.id = rxId;
                frame.isExtended = (ext != 0);
                frame.dlc = len;
                memcpy(frame.data, rxBuf, len > 8 ? 8 : len);
                return true;
            }
        }
        delay(1);
    }
    return false;
#else
    (void)frame;
    (void)timeoutMs;
    return false;
#endif
}
