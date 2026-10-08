#include "OBDManager.h"
#include "../system/Logger.h"
#include "../system/ErrorManager.h"
#include "../config/AppConfig.h"
#include <string.h>

OBDManager::OBDManager(CANManager* canManager)
    : m_canManager(canManager),
      m_status(OBDStatus::DISCONNECTED),
      m_isSimulation(false),
      m_supportedMask01_20(0),
      m_supportedMask21_40(0) {
    memset(m_pidFailureCount, 0, sizeof(m_pidFailureCount));
    memset(m_pidActiveSupport, 0, sizeof(m_pidActiveSupport));
}

bool OBDManager::init(CANManager* canManager, bool simulationMode) {
    m_canManager = canManager;
    m_isSimulation = simulationMode;
    m_status = OBDStatus::INITIALIZING;

    memset(m_pidFailureCount, 0, sizeof(m_pidFailureCount));
    memset(m_pidActiveSupport, 0, sizeof(m_pidActiveSupport));
    m_supportedMask01_20 = 0;
    m_supportedMask21_40 = 0;

    if (m_isSimulation) {
        Logger::logOBD("Operating in SIMULATION mode");
        // In simulation, simulate all standard telemetry PIDs as supported
        m_supportedMask01_20 = 0xBE1FA813; // Typical modern vehicle supported mask
        m_supportedMask21_40 = 0x80002001; // Supports PID 0x2F (Fuel Level)
        m_pidActiveSupport[OBD::PID_ENGINE_RPM] = true;
        m_pidActiveSupport[OBD::PID_VEHICLE_SPEED] = true;
        m_pidActiveSupport[OBD::PID_THROTTLE_POS] = true;
        m_pidActiveSupport[OBD::PID_ENGINE_LOAD] = true;
        m_pidActiveSupport[OBD::PID_MAF_AIR_FLOW] = true;
        m_pidActiveSupport[OBD::PID_COOLANT_TEMP] = true;
        m_pidActiveSupport[OBD::PID_INTAKE_TEMP] = true;
        m_pidActiveSupport[OBD::PID_FUEL_LEVEL] = true;

        m_status = OBDStatus::READY;
        Logger::logOBD("Vehicle detected (Simulated ECU connected)");
        Logger::logOBD("Supported PIDs: RPM, SPEED, THROTTLE, LOAD, MAF, COOLANT, IAT, FUEL");
        return true;
    }

    if (!m_canManager) {
        m_status = OBDStatus::ERROR_COMMUNICATION;
        return false;
    }

    return detectVehicle();
}

void OBDManager::setSimulation(bool enable) {
    m_isSimulation = enable;
    if (enable) {
        m_status = OBDStatus::READY;
        m_pidActiveSupport[OBD::PID_ENGINE_RPM] = true;
        m_pidActiveSupport[OBD::PID_VEHICLE_SPEED] = true;
        m_pidActiveSupport[OBD::PID_THROTTLE_POS] = true;
        m_pidActiveSupport[OBD::PID_ENGINE_LOAD] = true;
        m_pidActiveSupport[OBD::PID_MAF_AIR_FLOW] = true;
        m_pidActiveSupport[OBD::PID_COOLANT_TEMP] = true;
        m_pidActiveSupport[OBD::PID_INTAKE_TEMP] = true;
        m_pidActiveSupport[OBD::PID_FUEL_LEVEL] = true;
    }
}

bool OBDManager::detectVehicle() {
    if (m_isSimulation) {
        m_status = OBDStatus::READY;
        return true;
    }

    m_status = OBDStatus::DETECTING_VEHICLE;
    Logger::logOBD("Detecting Vehicle ECU on CAN bus (Broadcast 0x7DF)...");

    // Ping vehicle with PID 0x00 (Supported PIDs 01-20)
    OBDParseResult result;
    bool received = sendDiagnosticQuery(OBD::MODE_CURRENT_DATA, OBD::PID_SUPPORTED_01_20, result);

    if (received && result.isValid) {
        m_supportedMask01_20 = result.rawBitmask;
        Logger::logOBD("Vehicle Response Received! Supported PIDs [01-20] Mask: 0x%08X", m_supportedMask01_20);
        return discoverSupportedPIDs();
    } else {
        Logger::warn("OBD", "No response from vehicle ECU. Is ignition ON?");
        m_status = OBDStatus::NO_RESPONSE;
        ErrorManager::recordError(ErrorCode::OBD_TIMEOUT);
        return false;
    }
}

bool OBDManager::discoverSupportedPIDs() {
    if (m_isSimulation) {
        return true;
    }

    m_status = OBDStatus::DISCOVERING_PIDS;
    Logger::logOBD("Building adaptive polling schedule from vehicle supported PIDs...");

    // Check if PID 0x20 is supported to fetch PIDs [21-40]
    if (OBDParser::isPidSupportedInMask(0x00, OBD::PID_SUPPORTED_21_40, m_supportedMask01_20)) {
        OBDParseResult res20;
        if (sendDiagnosticQuery(OBD::MODE_CURRENT_DATA, OBD::PID_SUPPORTED_21_40, res20) && res20.isValid) {
            m_supportedMask21_40 = res20.rawBitmask;
            Logger::logOBD("Supported PIDs [21-40] Mask: 0x%08X", m_supportedMask21_40);
        }
    }

    // Inspect individual parameters
    size_t count = OBDPIDRegistry::getDescriptorCount();
    for (size_t i = 0; i < count; ++i) {
        const OBDPIDDescriptor* desc = OBDPIDRegistry::getDescriptorByIndex(i);
        if (!desc) continue;

        bool supported = false;
        if (desc->pid <= 0x20) {
            supported = OBDParser::isPidSupportedInMask(0x00, desc->pid, m_supportedMask01_20);
        } else if (desc->pid <= 0x40) {
            supported = OBDParser::isPidSupportedInMask(0x20, desc->pid, m_supportedMask21_40);
        }

        if (desc->pid < sizeof(m_pidActiveSupport)) {
            m_pidActiveSupport[desc->pid] = supported;
        }

        if (supported) {
            Logger::logOBD("Supported PID: %s (0x%02X)", desc->name, desc->pid);
        } else {
            Logger::debug("OBD", "Unsupported PID on this vehicle: %s (0x%02X)", desc->name, desc->pid);
        }
    }

    m_status = OBDStatus::READY;
    return true;
}

bool OBDManager::sendDiagnosticQuery(uint8_t mode, uint8_t pid, OBDParseResult& outResult) {
    if (!m_canManager) return false;

    uint8_t txBuffer[8];
    OBDParser::buildRequestFrame(mode, pid, txBuffer);
    CANFrame txFrame(OBD_CAN_BROADCAST_ID, 8, txBuffer, false);

    if (!m_canManager->sendFrame(txFrame)) {
        ErrorManager::recordError(ErrorCode::CAN_TRANSMIT_FAILED);
        return false;
    }

    // Receive response with timeout
    CANFrame rxFrame;
    if (m_canManager->receiveFrame(rxFrame, OBD_RESPONSE_TIMEOUT_MS)) {
        // Validate that response CAN ID is an ECU diagnostic response (0x7E8 - 0x7EF)
        if ((rxFrame.id & OBD_ECU_RESPONSE_MASK) == 0x7E8 || rxFrame.id == 0x18DAF110) {
            outResult = OBDParser::parseFrame(rxFrame.data, rxFrame.dlc, pid);
            return outResult.isValid;
        }
    }

    return false;
}

bool OBDManager::isPIDSupported(uint8_t pid) const {
    if (pid < sizeof(m_pidActiveSupport)) {
        return m_pidActiveSupport[pid];
    }
    return false;
}

bool OBDManager::queryPID(uint8_t pid, float& outValue) {
    // If not supported, do NOT overload diagnostic bus
    if (!isPIDSupported(pid)) {
        return false;
    }

    if (m_isSimulation) {
        // In simulation mode, values are populated by TelemetryManager's driving model
        return true;
    }

    OBDParseResult result;
    bool success = sendDiagnosticQuery(OBD::MODE_CURRENT_DATA, pid, result);

    if (success && result.isValid) {
        outValue = result.convertedValue;
        if (pid < sizeof(m_pidFailureCount)) {
            m_pidFailureCount[pid] = 0; // Reset consecutive failures
        }
        return true;
    }

    // Error handling & retry backoff
    if (pid < sizeof(m_pidFailureCount)) {
        m_pidFailureCount[pid]++;
        if (m_pidFailureCount[pid] >= OBD_MAX_RETRIES) {
            Logger::warn("OBD", "PID 0x%02X failed %d times; temporarily disabling to protect CAN bus",
                         pid, m_pidFailureCount[pid]);
            m_pidActiveSupport[pid] = false;
            ErrorManager::recordError(ErrorCode::OBD_PID_UNSUPPORTED);
        }
    }

    return false;
}
