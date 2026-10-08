#pragma once

#include "SupportedPIDs.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @file OBDParser.h
 * @brief Parser and formula calculator for ISO 15765-4 OBD-II responses.
 */

struct OBDParseResult {
    bool isValid;
    uint8_t mode;
    uint8_t pid;
    float convertedValue;
    uint32_t rawBitmask; // Used for PIDs 0x00, 0x20, 0x40
    bool isNegativeResponse;
    uint8_t nrc; // Negative response code if isNegativeResponse is true
};

class OBDParser {
public:
    /**
     * @brief Validate and parse raw CAN data frame from ECU.
     * @param data Array of 8 CAN payload bytes.
     * @param len Number of valid CAN bytes (typically 8).
     * @param expectedPid The PID that was requested.
     * @return OBDParseResult containing parsed float or bitmask.
     */
    static OBDParseResult parseFrame(const uint8_t* data, size_t len, uint8_t expectedPid);

    /**
     * @brief Build standard 8-byte diagnostic request frame for a PID.
     * @param mode Diagnostic mode (typically 0x01).
     * @param pid Parameter ID.
     * @param outBuffer Destination 8-byte buffer.
     */
    static void buildRequestFrame(uint8_t mode, uint8_t pid, uint8_t outBuffer[8]);

    /**
     * @brief Check if a target PID is reported supported in a 32-bit bitmask response.
     * @param basePid 0x00, 0x20, or 0x40
     * @param targetPid The PID to check
     * @param bitmask 32-bit mask decoded from ECU response
     */
    static bool isPidSupportedInMask(uint8_t basePid, uint8_t targetPid, uint32_t bitmask);

    // Individual conversion formulas
    static float calculateRPM(uint8_t a, uint8_t b);
    static float calculateSpeed(uint8_t a);
    static float calculateCoolantTemp(uint8_t a);
    static float calculateEngineLoad(uint8_t a);
    static float calculateThrottlePos(uint8_t a);
    static float calculateIntakeTemp(uint8_t a);
    static float calculateMAF(uint8_t a, uint8_t b);
    static float calculateFuelLevel(uint8_t a);
};
