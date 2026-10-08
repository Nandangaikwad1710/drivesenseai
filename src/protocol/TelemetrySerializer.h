#pragma once

#include "../telemetry/TelemetryData.h"
#include <stddef.h>

/**
 * @file TelemetrySerializer.h
 * @brief Abstract serialization interface for live telemetry streams.
 */

class TelemetrySerializer {
public:
    virtual ~TelemetrySerializer() = default;

    /**
     * @brief Serialize a TelemetryData instance into a destination buffer.
     * @param data The normalized telemetry object.
     * @param outBuffer Destination character/byte buffer.
     * @param maxLen Maximum capacity of the destination buffer.
     * @return Number of bytes written (excluding null terminator if any), or 0 on error.
     */
    virtual size_t serialize(const TelemetryData& data, char* outBuffer, size_t maxLen) = 0;
};
