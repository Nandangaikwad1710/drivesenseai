#pragma once

#include <stdint.h>
#include <stddef.h>

/**
 * @file SystemMonitor.h
 * @brief FreeRTOS task health, memory monitoring, and watchdog supervisor.
 */

struct SystemHealth {
    uint32_t uptimeSeconds;
    uint32_t freeHeapBytes;
    uint32_t minFreeHeapBytes;
    bool obdTaskHealthy;
    bool telemetryTaskHealthy;
    bool bleTaskHealthy;
};

class SystemMonitor {
public:
    static void init();
    static void feedWatchdog();
    static void reportHeartbeat(const char* taskName);
    static SystemHealth getHealth();
    static void printStatus();

private:
    static uint32_t s_bootTimestampMs;
    static uint32_t s_lastOBDHeartbeat;
    static uint32_t s_lastTelemHeartbeat;
    static uint32_t s_lastBLEHeartbeat;
};
