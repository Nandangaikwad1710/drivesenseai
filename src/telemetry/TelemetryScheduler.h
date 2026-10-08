#pragma once

#include <stdint.h>
#include "../config/AppConfig.h"

/**
 * @file TelemetryScheduler.h
 * @brief Dynamic task and parameter polling scheduler.
 */

class TelemetryScheduler {
public:
    TelemetryScheduler()
        : m_lastHighPollMs(0),
          m_lastMediumPollMs(0),
          m_lastLowPollMs(0),
          m_lastStreamMs(0) {}

    bool isTimeToPollHigh(uint32_t nowMs) {
        if (nowMs - m_lastHighPollMs >= POLLING_INTERVAL_HIGH_MS) {
            m_lastHighPollMs = nowMs;
            return true;
        }
        return false;
    }

    bool isTimeToPollMedium(uint32_t nowMs) {
        if (nowMs - m_lastMediumPollMs >= POLLING_INTERVAL_MEDIUM_MS) {
            m_lastMediumPollMs = nowMs;
            return true;
        }
        return false;
    }

    bool isTimeToPollLow(uint32_t nowMs) {
        if (nowMs - m_lastLowPollMs >= POLLING_INTERVAL_LOW_MS) {
            m_lastLowPollMs = nowMs;
            return true;
        }
        return false;
    }

    bool isTimeToStream(uint32_t nowMs) {
        if (nowMs - m_lastStreamMs >= TELEMETRY_STREAM_INTERVAL_MS) {
            m_lastStreamMs = nowMs;
            return true;
        }
        return false;
    }

    void reset(uint32_t nowMs) {
        m_lastHighPollMs = nowMs;
        m_lastMediumPollMs = nowMs;
        m_lastLowPollMs = nowMs;
        m_lastStreamMs = nowMs;
    }

private:
    uint32_t m_lastHighPollMs;
    uint32_t m_lastMediumPollMs;
    uint32_t m_lastLowPollMs;
    uint32_t m_lastStreamMs;
};
