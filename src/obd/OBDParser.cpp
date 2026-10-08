#include "OBDParser.h"
#include <string.h>

void OBDParser::buildRequestFrame(uint8_t mode, uint8_t pid, uint8_t outBuffer[8]) {
    // ISO 15765-4 Single Frame request
    // Byte 0: Number of additional data bytes (0x02: Mode + PID)
    // Byte 1: Service / Mode (e.g. 0x01)
    // Byte 2: Parameter ID (e.g. 0x0C)
    // Bytes 3-7: Standard padding (0x55 or 0xAA)
    outBuffer[0] = 0x02;
    outBuffer[1] = mode;
    outBuffer[2] = pid;
    outBuffer[3] = 0x55;
    outBuffer[4] = 0x55;
    outBuffer[5] = 0x55;
    outBuffer[6] = 0x55;
    outBuffer[7] = 0x55;
}

OBDParseResult OBDParser::parseFrame(const uint8_t* data, size_t len, uint8_t expectedPid) {
    OBDParseResult res = {};
    res.isValid = false;
    res.isNegativeResponse = false;

    if (!data || len < 3) {
        return res;
    }

    uint8_t payloadBytes = data[0];
    if (payloadBytes < 2 || payloadBytes > 7) {
        return res;
    }

    uint8_t respMode = data[1];

    // Check for Negative Response (0x7F)
    if (respMode == OBD::MODE_NEGATIVE_RESP) {
        res.isNegativeResponse = true;
        if (payloadBytes >= 3) {
            res.mode = data[2]; // Requested service
            res.nrc = data[3];  // Negative response code
        }
        return res;
    }

    // Check for positive response (Mode + 0x40)
    if (respMode != OBD::MODE_CURRENT_DATA_RESP) {
        return res;
    }

    uint8_t respPid = data[2];
    if (respPid != expectedPid) {
        return res;
    }

    res.mode = respMode;
    res.pid = respPid;

    // Handle supported PID bitmask queries (PIDs 0x00, 0x20, 0x40)
    if (respPid == OBD::PID_SUPPORTED_01_20 || respPid == OBD::PID_SUPPORTED_21_40 || respPid == OBD::PID_SUPPORTED_41_60) {
        if (payloadBytes >= 6) { // Byte 0 = len, Byte 1 = Mode, Byte 2 = PID, Bytes 3..6 = 4 data bytes
            res.rawBitmask = ((uint32_t)data[3] << 24) |
                             ((uint32_t)data[4] << 16) |
                             ((uint32_t)data[5] << 8)  |
                             ((uint32_t)data[6]);
            res.isValid = true;
        }
        return res;
    }

    // Handle standard sensor telemetry
    switch (respPid) {
        case OBD::PID_ENGINE_RPM: // ((A * 256) + B) / 4
            if (payloadBytes >= 4) {
                res.convertedValue = calculateRPM(data[3], data[4]);
                res.isValid = true;
            }
            break;

        case OBD::PID_VEHICLE_SPEED: // A km/h
            if (payloadBytes >= 3) {
                res.convertedValue = calculateSpeed(data[3]);
                res.isValid = true;
            }
            break;

        case OBD::PID_THROTTLE_POS: // (A * 100) / 255
            if (payloadBytes >= 3) {
                res.convertedValue = calculateThrottlePos(data[3]);
                res.isValid = true;
            }
            break;

        case OBD::PID_ENGINE_LOAD: // (A * 100) / 255
            if (payloadBytes >= 3) {
                res.convertedValue = calculateEngineLoad(data[3]);
                res.isValid = true;
            }
            break;

        case OBD::PID_MAF_AIR_FLOW: // ((A * 256) + B) / 100
            if (payloadBytes >= 4) {
                res.convertedValue = calculateMAF(data[3], data[4]);
                res.isValid = true;
            }
            break;

        case OBD::PID_COOLANT_TEMP: // A - 40
            if (payloadBytes >= 3) {
                res.convertedValue = calculateCoolantTemp(data[3]);
                res.isValid = true;
            }
            break;

        case OBD::PID_INTAKE_TEMP: // A - 40
            if (payloadBytes >= 3) {
                res.convertedValue = calculateIntakeTemp(data[3]);
                res.isValid = true;
            }
            break;

        case OBD::PID_FUEL_LEVEL: // (A * 100) / 255
            if (payloadBytes >= 3) {
                res.convertedValue = calculateFuelLevel(data[3]);
                res.isValid = true;
            }
            break;

        default:
            res.isValid = false;
            break;
    }

    return res;
}

bool OBDParser::isPidSupportedInMask(uint8_t basePid, uint8_t targetPid, uint32_t bitmask) {
    if (targetPid <= basePid || targetPid > (basePid + 32)) {
        return false;
    }
    uint8_t offset = targetPid - basePid; // 1 .. 32
    uint8_t shift = 32 - offset;
    return (bitmask & (1UL << shift)) != 0;
}

float OBDParser::calculateRPM(uint8_t a, uint8_t b) {
    return ((float)((uint16_t)a * 256 + b)) / 4.0f;
}

float OBDParser::calculateSpeed(uint8_t a) {
    return (float)a;
}

float OBDParser::calculateCoolantTemp(uint8_t a) {
    return (float)a - 40.0f;
}

float OBDParser::calculateEngineLoad(uint8_t a) {
    return ((float)a * 100.0f) / 255.0f;
}

float OBDParser::calculateThrottlePos(uint8_t a) {
    return ((float)a * 100.0f) / 255.0f;
}

float OBDParser::calculateIntakeTemp(uint8_t a) {
    return (float)a - 40.0f;
}

float OBDParser::calculateMAF(uint8_t a, uint8_t b) {
    return ((float)((uint16_t)a * 256 + b)) / 100.0f;
}

float OBDParser::calculateFuelLevel(uint8_t a) {
    return ((float)a * 100.0f) / 255.0f;
}
