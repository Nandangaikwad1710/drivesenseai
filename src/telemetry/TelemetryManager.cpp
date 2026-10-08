#include "TelemetryManager.h"
#include "../system/Logger.h"
#include "../config/AppConfig.h"
#include <math.h>

TelemetryManager::TelemetryManager(OBDManager* obdManager)
    : m_obdManager(obdManager),
      m_isStreaming(false),
      m_simulationMode(true),
      m_simPhase(DrivingPhase::IDLE),
      m_simPhaseStartMs(0),
      m_simInitialized(false),
      m_simSpeed(0.0f),
      m_simRpm(800.0f),
      m_simThrottle(0.0f),
      m_simLoad(18.0f),
      m_simCoolant(45.0f),
      m_simIAT(28.0f),
      m_simMaf(2.8f),
      m_simFuel(68.5f) {}

bool TelemetryManager::init(OBDManager* obdManager, bool simulationMode) {
    m_obdManager = obdManager;
    m_simulationMode = simulationMode;
    m_isStreaming = false;
    m_simPhase = DrivingPhase::IDLE;
    m_simPhaseStartMs = 0;
    m_simInitialized = false;
    m_currentData.reset();

    Logger::logTelemetry("TelemetryManager initialized (Simulation: %s)",
                         m_simulationMode ? "ENABLED" : "DISABLED");
    return true;
}

void TelemetryManager::startStreaming() {
    m_isStreaming = true;
    Logger::logTelemetry("Streaming Started");
}

void TelemetryManager::stopStreaming() {
    m_isStreaming = false;
    Logger::logTelemetry("Streaming Stopped");
}

void TelemetryManager::setSimulation(bool enable) {
    m_simulationMode = enable;
    if (m_obdManager) {
        m_obdManager->setSimulation(enable);
    }
    Logger::logTelemetry("Simulation mode toggled to: %s", enable ? "TRUE" : "FALSE");
}

void TelemetryManager::update(uint32_t nowMs) {
    if (m_simulationMode) {
        stepSimulation(nowMs);
    } else {
        pollRealVehicle(nowMs);
    }

    // When streaming, push sample to buffer at designated interval
    if (m_isStreaming && m_scheduler.isTimeToStream(nowMs)) {
        m_currentData.timestamp = (uint64_t)nowMs;
        m_buffer.push(m_currentData);
    }
}

void TelemetryManager::stepSimulation(uint32_t nowMs) {
    if (!m_simInitialized) {
        m_simPhaseStartMs = nowMs;
        m_simInitialized = true;
    }

    uint32_t elapsed = nowMs - m_simPhaseStartMs;

    // Check phase transitions
    if (m_simPhase == DrivingPhase::IDLE && elapsed >= 4000) {
        m_simPhase = DrivingPhase::ACCELERATING;
        m_simPhaseStartMs = nowMs;
        elapsed = 0;
        Logger::logTelemetry("[SIM] Transition: Idle -> Accelerating");
    } else if (m_simPhase == DrivingPhase::ACCELERATING && elapsed >= 7000) {
        m_simPhase = DrivingPhase::CRUISING;
        m_simPhaseStartMs = nowMs;
        elapsed = 0;
        Logger::logTelemetry("[SIM] Transition: Accelerating -> Cruising");
    } else if (m_simPhase == DrivingPhase::CRUISING && elapsed >= 8000) {
        m_simPhase = DrivingPhase::DECELERATING;
        m_simPhaseStartMs = nowMs;
        elapsed = 0;
        Logger::logTelemetry("[SIM] Transition: Cruising -> Decelerating");
    } else if (m_simPhase == DrivingPhase::DECELERATING && elapsed >= 5000) {
        m_simPhase = DrivingPhase::IDLE;
        m_simPhaseStartMs = nowMs;
        elapsed = 0;
        Logger::logTelemetry("[SIM] Transition: Decelerating -> Idle");
    }

    // Evaluate current driving phase physics
    switch (m_simPhase) {
        case DrivingPhase::IDLE: {
            m_simThrottle = 0.0f;
            m_simRpm = 800.0f + 20.0f * sinf((float)nowMs / 300.0f);
            m_simSpeed = 0.0f;
            m_simLoad = 18.0f + 2.0f * sinf((float)nowMs / 500.0f);
            m_simMaf = 2.8f;
            break;
        }

        case DrivingPhase::ACCELERATING: {
            float progress = (float)elapsed / 7000.0f;
            if (progress > 1.0f) progress = 1.0f;

            m_simThrottle = 35.0f + 10.0f * progress;
            m_simSpeed = progress * 65.0f; // 0 -> 65 km/h
            m_simRpm = 800.0f + (progress * 2600.0f); // 800 -> 3400 RPM
            m_simLoad = 50.0f + (progress * 25.0f);   // 50% -> 75%
            m_simMaf = 3.0f + (progress * 28.0f);     // 3 -> 31 g/s
            break;
        }

        case DrivingPhase::CRUISING: {
            m_simThrottle = 18.0f + 3.0f * sinf((float)nowMs / 800.0f);
            m_simRpm = 2050.0f + 80.0f * sinf((float)nowMs / 600.0f);
            m_simSpeed = 65.0f + 5.0f * sinf((float)nowMs / 1200.0f);
            m_simLoad = 38.0f + 4.0f * sinf((float)nowMs / 700.0f);
            m_simMaf = 10.5f + 1.5f * sinf((float)nowMs / 600.0f);
            break;
        }

        case DrivingPhase::DECELERATING: {
            float progress = (float)elapsed / 5000.0f;
            if (progress > 1.0f) progress = 1.0f;

            m_simThrottle = 0.0f;
            m_simSpeed = 65.0f * (1.0f - progress);
            m_simRpm = 2000.0f - (progress * 1200.0f); // 2000 -> 800 RPM
            m_simLoad = 20.0f - (progress * 5.0f);
            m_simMaf = 8.0f - (progress * 5.2f);
            break;
        }
    }

    // Engine warmup: coolant slowly climbs to 90°C
    if (m_simCoolant < 90.0f) {
        m_simCoolant += 0.02f;
    }
    // Ambient intake temp around 28°C
    m_simIAT = 28.0f + 1.0f * sinf((float)nowMs / 4000.0f);
    // Slow fuel consumption
    if (m_simFuel > 10.0f) {
        m_simFuel -= 0.0005f;
    }

    // Populate current normalized telemetry object
    m_currentData.speed = m_simSpeed;
    m_currentData.speedAvailable = true;

    m_currentData.rpm = m_simRpm;
    m_currentData.rpmAvailable = true;

    m_currentData.throttlePosition = m_simThrottle;
    m_currentData.throttleAvailable = true;

    m_currentData.engineLoad = m_simLoad;
    m_currentData.engineLoadAvailable = true;

    m_currentData.massAirFlow = m_simMaf;
    m_currentData.mafAvailable = true;

    m_currentData.coolantTemperature = m_simCoolant;
    m_currentData.coolantTemperatureAvailable = true;

    m_currentData.intakeAirTemperature = m_simIAT;
    m_currentData.intakeAirTemperatureAvailable = true;

    m_currentData.fuelLevel = m_simFuel;
    m_currentData.fuelLevelAvailable = true;
}

void TelemetryManager::pollRealVehicle(uint32_t nowMs) {
    if (!m_obdManager) return;

    // 1. High Priority parameters (Speed, RPM, Throttle)
    if (m_scheduler.isTimeToPollHigh(nowMs)) {
        float val = 0.0f;
        if (m_obdManager->queryPID(OBD::PID_VEHICLE_SPEED, val)) {
            m_currentData.speed = val;
            m_currentData.speedAvailable = true;
        }
        if (m_obdManager->queryPID(OBD::PID_ENGINE_RPM, val)) {
            m_currentData.rpm = val;
            m_currentData.rpmAvailable = true;
        }
        if (m_obdManager->queryPID(OBD::PID_THROTTLE_POS, val)) {
            m_currentData.throttlePosition = val;
            m_currentData.throttleAvailable = true;
        }
    }

    // 2. Medium Priority parameters (Engine Load, MAF)
    if (m_scheduler.isTimeToPollMedium(nowMs)) {
        float val = 0.0f;
        if (m_obdManager->queryPID(OBD::PID_ENGINE_LOAD, val)) {
            m_currentData.engineLoad = val;
            m_currentData.engineLoadAvailable = true;
        }
        if (m_obdManager->queryPID(OBD::PID_MAF_AIR_FLOW, val)) {
            m_currentData.massAirFlow = val;
            m_currentData.mafAvailable = true;
        }
    }

    // 3. Low Priority parameters (Coolant, IAT, Fuel Level)
    if (m_scheduler.isTimeToPollLow(nowMs)) {
        float val = 0.0f;
        if (m_obdManager->queryPID(OBD::PID_COOLANT_TEMP, val)) {
            m_currentData.coolantTemperature = val;
            m_currentData.coolantTemperatureAvailable = true;
        }
        if (m_obdManager->queryPID(OBD::PID_INTAKE_TEMP, val)) {
            m_currentData.intakeAirTemperature = val;
            m_currentData.intakeAirTemperatureAvailable = true;
        }
        if (m_obdManager->queryPID(OBD::PID_FUEL_LEVEL, val)) {
            m_currentData.fuelLevel = val;
            m_currentData.fuelLevelAvailable = true;
        }
    }
}
