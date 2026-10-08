#pragma once

#include <stdint.h>
#include <stddef.h>

/**
 * @file ErrorManager.h
 * @brief Error tracking, classification, and diagnostics for DriveSense OBD.
 */

enum class ErrorCode : uint16_t {
    NONE                  = 0,
    CAN_INIT_FAILED       = 101,
    CAN_TRANSMIT_FAILED   = 102,
    CAN_BUS_OFF           = 103,
    CAN_FRAME_ERROR       = 104,
    OBD_TIMEOUT           = 201,
    OBD_INVALID_RESPONSE  = 202,
    OBD_PID_UNSUPPORTED   = 203,
    OBD_NO_DATA           = 204,
    BLE_INIT_FAILED       = 301,
    BLE_DISCONNECTED      = 302,
    BLE_CONGESTION        = 303,
    QUEUE_OVERFLOW        = 401,
    MEMORY_LOW            = 402,
    WATCHDOG_TRIGGERED    = 403
};

struct ErrorStats {
    uint32_t canTransmitErrors;
    uint32_t obdTimeouts;
    uint32_t obdInvalidResponses;
    uint32_t bleDisconnects;
    uint32_t queueDrops;
    ErrorCode lastError;
    uint32_t lastErrorTimestamp;
};

class ErrorManager {
public:
    static void init() {
        s_stats = {0, 0, 0, 0, 0, ErrorCode::NONE, 0};
    }

    static void recordError(ErrorCode code, uint32_t timestamp = 0) {
        s_stats.lastError = code;
        s_stats.lastErrorTimestamp = timestamp;

        switch (code) {
            case ErrorCode::CAN_TRANSMIT_FAILED:
            case ErrorCode::CAN_BUS_OFF:
            case ErrorCode::CAN_FRAME_ERROR:
                s_stats.canTransmitErrors++;
                break;
            case ErrorCode::OBD_TIMEOUT:
                s_stats.obdTimeouts++;
                break;
            case ErrorCode::OBD_INVALID_RESPONSE:
                s_stats.obdInvalidResponses++;
                break;
            case ErrorCode::BLE_DISCONNECTED:
                s_stats.bleDisconnects++;
                break;
            case ErrorCode::QUEUE_OVERFLOW:
                s_stats.queueDrops++;
                break;
            default:
                break;
        }
    }

    static const ErrorStats& getStats() {
        return s_stats;
    }

    static const char* getErrorString(ErrorCode code) {
        switch (code) {
            case ErrorCode::NONE: return "NO_ERROR";
            case ErrorCode::CAN_INIT_FAILED: return "CAN_INIT_FAILED";
            case ErrorCode::CAN_TRANSMIT_FAILED: return "CAN_TRANSMIT_FAILED";
            case ErrorCode::CAN_BUS_OFF: return "CAN_BUS_OFF";
            case ErrorCode::CAN_FRAME_ERROR: return "CAN_FRAME_ERROR";
            case ErrorCode::OBD_TIMEOUT: return "OBD_TIMEOUT";
            case ErrorCode::OBD_INVALID_RESPONSE: return "OBD_INVALID_RESPONSE";
            case ErrorCode::OBD_PID_UNSUPPORTED: return "OBD_PID_UNSUPPORTED";
            case ErrorCode::OBD_NO_DATA: return "OBD_NO_DATA";
            case ErrorCode::BLE_INIT_FAILED: return "BLE_INIT_FAILED";
            case ErrorCode::BLE_DISCONNECTED: return "BLE_DISCONNECTED";
            case ErrorCode::BLE_CONGESTION: return "BLE_CONGESTION";
            case ErrorCode::QUEUE_OVERFLOW: return "QUEUE_OVERFLOW";
            case ErrorCode::MEMORY_LOW: return "MEMORY_LOW";
            case ErrorCode::WATCHDOG_TRIGGERED: return "WATCHDOG_TRIGGERED";
            default: return "UNKNOWN_ERROR";
        }
    }

private:
    inline static ErrorStats s_stats = {0, 0, 0, 0, 0, ErrorCode::NONE, 0};
};
