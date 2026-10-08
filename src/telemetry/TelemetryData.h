#pragma once

#include <stdint.h>
#include <string.h>
#include "../config/AppConfig.h"

#if defined(ARDUINO) && !defined(DRIVESENSE_TEST_SUITE)
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#else
#include <mutex>
#endif

/**
 * @file TelemetryData.h
 * @brief Normalized vehicle telemetry data model and thread-safe circular buffer.
 */

struct TelemetryData {
    uint64_t timestamp; // Unix epoch or milliseconds since boot

    float speed;
    bool speedAvailable;

    float rpm;
    bool rpmAvailable;

    float engineLoad;
    bool engineLoadAvailable;

    float throttlePosition;
    bool throttleAvailable;

    float coolantTemperature;
    bool coolantTemperatureAvailable;

    float intakeAirTemperature;
    bool intakeAirTemperatureAvailable;

    float massAirFlow;
    bool mafAvailable;

    float fuelLevel;
    bool fuelLevelAvailable;

    TelemetryData() {
        reset();
    }

    void reset() {
        timestamp = 0;
        speed = 0.0f;
        speedAvailable = false;
        rpm = 0.0f;
        rpmAvailable = false;
        engineLoad = 0.0f;
        engineLoadAvailable = false;
        throttlePosition = 0.0f;
        throttleAvailable = false;
        coolantTemperature = 0.0f;
        coolantTemperatureAvailable = false;
        intakeAirTemperature = 0.0f;
        intakeAirTemperatureAvailable = false;
        massAirFlow = 0.0f;
        mafAvailable = false;
        fuelLevel = 0.0f;
        fuelLevelAvailable = false;
    }
};

/**
 * @class TelemetryBuffer
 * @brief Thread-safe circular buffer with oldest-discard policy for telemetry samples.
 */
class TelemetryBuffer {
public:
    TelemetryBuffer() : m_head(0), m_tail(0), m_count(0), m_droppedCount(0) {
#if defined(ARDUINO) && !defined(DRIVESENSE_TEST_SUITE)
        m_mutex = xSemaphoreCreateMutex();
#endif
    }

    ~TelemetryBuffer() {
#if defined(ARDUINO) && !defined(DRIVESENSE_TEST_SUITE)
        if (m_mutex) {
            vSemaphoreDelete(m_mutex);
            m_mutex = nullptr;
        }
#endif
    }

    bool push(const TelemetryData& item) {
        lock();
        bool droppedOldest = false;

        if (m_count == TELEMETRY_BUFFER_CAPACITY) {
            // Buffer full: discard oldest item to prevent unbounded memory growth
            m_tail = (m_tail + 1) % TELEMETRY_BUFFER_CAPACITY;
            m_count--;
            m_droppedCount++;
            droppedOldest = true;
        }

        m_buffer[m_head] = item;
        m_head = (m_head + 1) % TELEMETRY_BUFFER_CAPACITY;
        m_count++;

        unlock();
        return !droppedOldest;
    }

    bool pop(TelemetryData& outItem) {
        lock();
        if (m_count == 0) {
            unlock();
            return false;
        }

        outItem = m_buffer[m_tail];
        m_tail = (m_tail + 1) % TELEMETRY_BUFFER_CAPACITY;
        m_count--;

        unlock();
        return true;
    }

    bool peekLatest(TelemetryData& outItem) {
        lock();
        if (m_count == 0) {
            unlock();
            return false;
        }

        size_t latestIdx = (m_head == 0) ? (TELEMETRY_BUFFER_CAPACITY - 1) : (m_head - 1);
        outItem = m_buffer[latestIdx];

        unlock();
        return true;
    }

    size_t size() {
        lock();
        size_t c = m_count;
        unlock();
        return c;
    }

    bool isEmpty() {
        return size() == 0;
    }

    uint32_t getDroppedCount() {
        lock();
        uint32_t drops = m_droppedCount;
        unlock();
        return drops;
    }

    void clear() {
        lock();
        m_head = 0;
        m_tail = 0;
        m_count = 0;
        unlock();
    }

private:
    void lock() {
#if defined(ARDUINO) && !defined(DRIVESENSE_TEST_SUITE)
        if (m_mutex) xSemaphoreTake(m_mutex, portMAX_DELAY);
#else
        m_nativeMutex.lock();
#endif
    }

    void unlock() {
#if defined(ARDUINO) && !defined(DRIVESENSE_TEST_SUITE)
        if (m_mutex) xSemaphoreGive(m_mutex);
#else
        m_nativeMutex.unlock();
#endif
    }

    TelemetryData m_buffer[TELEMETRY_BUFFER_CAPACITY];
    size_t m_head;
    size_t m_tail;
    size_t m_count;
    uint32_t m_droppedCount;

#if defined(ARDUINO) && !defined(DRIVESENSE_TEST_SUITE)
    SemaphoreHandle_t m_mutex;
#else
    std::mutex m_nativeMutex;
#endif
};
