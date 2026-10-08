#pragma once

#include "TelemetrySerializer.h"

/**
 * @file BinarySerializer.h
 * @brief High-efficiency packed binary protocol serializer for low-latency BLE transmissions.
 */

#pragma pack(push, 1)
struct BinaryTelemetryPacket {
    uint8_t header[2];       // Magic 0xAA, 0x55
    uint16_t availableFlags; // Bitmask of available fields
    uint32_t timestampMs;    // Timestamp
    uint16_t speedX10;       // Speed * 10 (0.1 km/h resolution)
    uint16_t rpmX4;          // RPM * 4
    uint16_t throttleX10;    // Throttle * 10
    uint16_t engineLoadX10;  // Load * 10
    int16_t coolantTempX10;  // Coolant * 10 (signed)
    int16_t intakeTempX10;   // IAT * 10 (signed)
    uint16_t mafX100;        // MAF * 100
    uint16_t fuelLevelX10;   // Fuel * 10
    uint8_t checksum;        // XOR checksum
};
#pragma pack(pop)

class BinarySerializer : public TelemetrySerializer {
public:
    size_t serialize(const TelemetryData& data, char* outBuffer, size_t maxLen) override;
};
