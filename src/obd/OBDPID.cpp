#include "OBDPID.h"
#include <stddef.h>

static const OBDPIDDescriptor s_descriptors[] = {
    { OBD::PID_ENGINE_RPM,    OBD::MODE_CURRENT_DATA, "Engine RPM",            "RPM",  PollingPriority::HIGH,   2, PIDSupportStatus::UNKNOWN, 0, 0 },
    { OBD::PID_VEHICLE_SPEED, OBD::MODE_CURRENT_DATA, "Vehicle Speed",         "km/h", PollingPriority::HIGH,   1, PIDSupportStatus::UNKNOWN, 0, 0 },
    { OBD::PID_THROTTLE_POS,  OBD::MODE_CURRENT_DATA, "Throttle Position",     "%",    PollingPriority::HIGH,   1, PIDSupportStatus::UNKNOWN, 0, 0 },
    { OBD::PID_ENGINE_LOAD,   OBD::MODE_CURRENT_DATA, "Engine Load",           "%",    PollingPriority::MEDIUM, 1, PIDSupportStatus::UNKNOWN, 0, 0 },
    { OBD::PID_MAF_AIR_FLOW,  OBD::MODE_CURRENT_DATA, "Mass Air Flow",         "g/s",  PollingPriority::MEDIUM, 2, PIDSupportStatus::UNKNOWN, 0, 0 },
    { OBD::PID_COOLANT_TEMP,  OBD::MODE_CURRENT_DATA, "Coolant Temperature",   "°C",   PollingPriority::LOW,    1, PIDSupportStatus::UNKNOWN, 0, 0 },
    { OBD::PID_INTAKE_TEMP,   OBD::MODE_CURRENT_DATA, "Intake Air Temperature","°C",   PollingPriority::LOW,    1, PIDSupportStatus::UNKNOWN, 0, 0 },
    { OBD::PID_FUEL_LEVEL,    OBD::MODE_CURRENT_DATA, "Fuel Level",            "%",    PollingPriority::LOW,    1, PIDSupportStatus::UNKNOWN, 0, 0 }
};

static const size_t s_descriptorCount = sizeof(s_descriptors) / sizeof(s_descriptors[0]);

const OBDPIDDescriptor* OBDPIDRegistry::getDescriptor(uint8_t pid) {
    for (size_t i = 0; i < s_descriptorCount; ++i) {
        if (s_descriptors[i].pid == pid) {
            return &s_descriptors[i];
        }
    }
    return nullptr;
}

size_t OBDPIDRegistry::getDescriptorCount() {
    return s_descriptorCount;
}

const OBDPIDDescriptor* OBDPIDRegistry::getDescriptorByIndex(size_t index) {
    if (index < s_descriptorCount) {
        return &s_descriptors[index];
    }
    return nullptr;
}
