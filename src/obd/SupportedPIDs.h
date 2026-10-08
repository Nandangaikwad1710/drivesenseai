#pragma once

#include <stdint.h>

/**
 * @file SupportedPIDs.h
 * @brief Standard SAE J1979 / ISO 15031-5 OBD-II Mode 01 Parameter IDs.
 */

namespace OBD {

// Modes
constexpr uint8_t MODE_CURRENT_DATA         = 0x01;
constexpr uint8_t MODE_FREEZE_FRAME         = 0x02;
constexpr uint8_t MODE_READ_DTC             = 0x03;
constexpr uint8_t MODE_CLEAR_DTC            = 0x04; // RESTRICTED IN READ-ONLY MODE
constexpr uint8_t MODE_VEHICLE_INFO         = 0x09;

// Expected response modes
constexpr uint8_t MODE_CURRENT_DATA_RESP    = 0x41;
constexpr uint8_t MODE_NEGATIVE_RESP        = 0x7F;

// Mode 01 Parameter IDs (PIDs)
constexpr uint8_t PID_SUPPORTED_01_20       = 0x00; // Bitmask of PIDs [01-20]
constexpr uint8_t PID_ENGINE_LOAD           = 0x04; // Calculated engine load (0-100%)
constexpr uint8_t PID_COOLANT_TEMP          = 0x05; // Engine coolant temperature (-40 to 215°C)
constexpr uint8_t PID_ENGINE_RPM            = 0x0C; // Engine speed (0 - 16383.75 RPM)
constexpr uint8_t PID_VEHICLE_SPEED         = 0x0D; // Vehicle speed (0 - 255 km/h)
constexpr uint8_t PID_INTAKE_TEMP           = 0x0F; // Intake air temperature (-40 to 215°C)
constexpr uint8_t PID_MAF_AIR_FLOW          = 0x10; // Mass air flow sensor (0 - 655.35 g/s)
constexpr uint8_t PID_THROTTLE_POS          = 0x11; // Throttle position (0 - 100%)
constexpr uint8_t PID_SUPPORTED_21_40       = 0x20; // Bitmask of PIDs [21-40]
constexpr uint8_t PID_FUEL_LEVEL            = 0x2F; // Fuel tank level input (0 - 100%)
constexpr uint8_t PID_SUPPORTED_41_60       = 0x40; // Bitmask of PIDs [41-60]

} // namespace OBD
