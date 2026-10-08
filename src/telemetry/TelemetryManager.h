#pragma once

#include "TelemetryData.h"
#include "TelemetryScheduler.h"
#include "../obd/OBDManager.h"
#include <stdint.h>

/**
 * @file TelemetryManager.h
 * @brief Coordinates vehicle sensor polling, simulation driving cycles,
 *        and normalized telemetry buffering.
 */

enum class DrivingPhase : uint8_t {
    IDLE = 0,
    ACCELERATING = 1,
    CRUISING = 2,
    DECELERATING = 3
};

class TelemetryManager {
public:
    TelemetryManager(OBDManager* obdManager = nullptr);

    bool init(OBDManager* obdManager, bool simulationMode = true);
    void update(uint32_t nowMs);

    void startStreaming();
    void stopStreaming();
    bool isStreaming() const { return m_isStreaming; }

    TelemetryBuffer& getBuffer() { return m_buffer; }
    const TelemetryData& getCurrentData() const { return m_currentData; }

    void setSimulation(bool enable);
    bool isSimulation() const { return m_simulationMode; }

private:
    OBDManager* m_obdManager;
    TelemetryBuffer m_buffer;
    TelemetryData m_currentData;
    TelemetryScheduler m_scheduler;

    bool m_isStreaming;
    bool m_simulationMode;

    // Realistic vehicle simulation variables
    DrivingPhase m_simPhase;
    uint32_t m_simPhaseStartMs;
    bool m_simInitialized;
    float m_simSpeed;
    float m_simRpm;
    float m_simThrottle;
    float m_simLoad;
    float m_simCoolant;
    float m_simIAT;
    float m_simMaf;
    float m_simFuel;

    void stepSimulation(uint32_t nowMs);
    void pollRealVehicle(uint32_t nowMs);
};
