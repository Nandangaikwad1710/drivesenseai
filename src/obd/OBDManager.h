#pragma once

#include "OBDPID.h"
#include "OBDParser.h"
#include "../can/CANManager.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @file OBDManager.h
 * @brief High-level OBD-II communication manager with vehicle detection,
 *        adaptive PID discovery, and fail-safe query engine.
 */

enum class OBDStatus : uint8_t {
    DISCONNECTED = 0,
    INITIALIZING = 1,
    DETECTING_VEHICLE = 2,
    DISCOVERING_PIDS = 3,
    READY = 4,
    NO_RESPONSE = 5,
    ERROR_COMMUNICATION = 6
};

class OBDManager {
public:
    OBDManager(CANManager* canManager = nullptr);

    bool init(CANManager* canManager, bool simulationMode = false);
    bool detectVehicle();
    bool discoverSupportedPIDs();
    bool queryPID(uint8_t pid, float& outValue);

    bool isPIDSupported(uint8_t pid) const;
    OBDStatus getStatus() const { return m_status; }
    uint32_t getSupportedMask0120() const { return m_supportedMask01_20; }
    uint32_t getSupportedMask2140() const { return m_supportedMask21_40; }

    void setSimulation(bool enable);
    bool isSimulation() const { return m_isSimulation; }

private:
    CANManager* m_canManager;
    OBDStatus m_status;
    bool m_isSimulation;

    uint32_t m_supportedMask01_20;
    uint32_t m_supportedMask21_40;

    // Per-PID tracking (indexed by PID 0x00 - 0x30)
    uint8_t m_pidFailureCount[0x40];
    bool m_pidActiveSupport[0x40];

    bool sendDiagnosticQuery(uint8_t mode, uint8_t pid, OBDParseResult& outResult);
};
