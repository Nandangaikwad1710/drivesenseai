#pragma once

#include <stdint.h>

/**
 * @file BLECommands.h
 * @brief Command definitions supported by DriveSense OBD via BLE.
 */

namespace BLECmd {

constexpr const char* START_TELEMETRY       = "START_TELEMETRY";
constexpr const char* STOP_TELEMETRY        = "STOP_TELEMETRY";
constexpr const char* GET_DEVICE_INFO       = "GET_DEVICE_INFO";
constexpr const char* GET_SUPPORTED_PIDS    = "GET_SUPPORTED_PIDS";
constexpr const char* GET_CONNECTION_STATUS = "GET_CONNECTION_STATUS";
constexpr const char* SET_SIMULATION_MODE   = "SET_SIMULATION_MODE";

} // namespace BLECmd

enum class CommandType : uint8_t {
    UNKNOWN = 0,
    START_TELEMETRY,
    STOP_TELEMETRY,
    GET_DEVICE_INFO,
    GET_SUPPORTED_PIDS,
    GET_CONNECTION_STATUS,
    SET_SIMULATION_MODE
};

struct CommandPayload {
    CommandType type;
    bool boolParam;
    int32_t intParam;
};
