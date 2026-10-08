#pragma once

#include <stdint.h>
#include <string.h>

/**
 * @file CANFrame.h
 * @brief Representation of standard/extended CAN 2.0A/B frames.
 */

struct CANFrame {
    uint32_t id;         // 11-bit standard ID (0x7DF) or 29-bit extended ID (0x18DB33F1)
    uint8_t dlc;         // Data length code (typically 8 for OBD-II)
    uint8_t data[8];     // Payload bytes
    bool isExtended;     // True for 29-bit CAN ID
    bool isRTR;          // Remote Transmission Request

    CANFrame() : id(0), dlc(0), isExtended(false), isRTR(false) {
        memset(data, 0, sizeof(data));
    }

    CANFrame(uint32_t frameId, uint8_t length, const uint8_t* payload, bool extended = false)
        : id(frameId), dlc(length > 8 ? 8 : length), isExtended(extended), isRTR(false) {
        memset(data, 0, sizeof(data));
        if (payload && dlc > 0) {
            memcpy(data, payload, dlc);
        }
    }
};
