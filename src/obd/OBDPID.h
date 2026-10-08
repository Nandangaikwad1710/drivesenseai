#pragma once

#include "SupportedPIDs.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @file OBDPID.h
 * @brief Representation and classification of individual OBD-II Parameter IDs.
 */

enum class PollingPriority : uint8_t {
    HIGH = 0,    // Queried every ~100-200ms (Speed, RPM, Throttle)
    MEDIUM = 1,  // Queried every ~500ms (Engine Load, MAF)
    LOW = 2      // Queried every ~5000ms (Coolant Temp, Intake Temp, Fuel Level)
};

enum class PIDSupportStatus : uint8_t {
    UNKNOWN = 0,
    SUPPORTED = 1,
    UNSUPPORTED = 2,
    TIMEOUT = 3
};

struct OBDPIDDescriptor {
    uint8_t pid;
    uint8_t mode;
    const char* name;
    const char* unit;
    PollingPriority priority;
    uint8_t expectedDataBytes;
    PIDSupportStatus status;
    uint32_t lastPolledTimestamp;
    uint8_t consecutiveFailures;
};

class OBDPIDRegistry {
public:
    static const OBDPIDDescriptor* getDescriptor(uint8_t pid);
    static size_t getDescriptorCount();
    static const OBDPIDDescriptor* getDescriptorByIndex(size_t index);
};
