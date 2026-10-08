#pragma once

#include <stdint.h>

/**
 * @file CANConfig.h
 * @brief CAN bus bitrates and controller configuration.
 */

enum class CANSpeed : uint8_t {
    CAN_250KBPS = 0, // Common on older CAN vehicles
    CAN_500KBPS = 1, // Standard ISO 15765-4 OBD-II speed on most modern vehicles
    CAN_1000KBPS = 2 // High-speed proprietary automotive networks
};

enum class CANClock : uint8_t {
    CLOCK_8MHZ = 8,
    CLOCK_16MHZ = 16,
    CLOCK_20MHZ = 20
};

enum class CANStatus : uint8_t {
    UNINITIALIZED,
    READY,
    ERROR_HARDWARE,
    BUS_OFF,
    SIMULATED
};
