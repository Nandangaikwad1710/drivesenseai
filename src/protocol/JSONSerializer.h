#pragma once

#include "TelemetrySerializer.h"

/**
 * @file JSONSerializer.h
 * @brief Serializes TelemetryData to JSON format with explicit null for unavailable sensors.
 */

class JSONSerializer : public TelemetrySerializer {
public:
    size_t serialize(const TelemetryData& data, char* outBuffer, size_t maxLen) override;
};
