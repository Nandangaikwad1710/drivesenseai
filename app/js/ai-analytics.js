/**
 * @file ai-analytics.js
 * @brief DriveSense AI trip analytics, fuel economy estimator, and eco-score engine.
 */

class AIAnalytics {
    constructor() {
        this.ecoScore = 90;
        this.accelScore = 88;
        this.throttleScore = 92;

        this.lastSpeed = 0;
        this.lastThrottle = 0;
        this.lastTimestamp = performance.now();

        this.aggressiveAccelEvents = 0;
        this.hardBrakingEvents = 0;
        this.idleSeconds = 0;

        this.lastInsightUpdate = 0;
        this.currentInsight = "Engine idle. Ready to initiate trip telemetry.";
    }

    reset() {
        this.ecoScore = 90;
        this.accelScore = 88;
        this.throttleScore = 92;
        this.aggressiveAccelEvents = 0;
        this.hardBrakingEvents = 0;
        this.idleSeconds = 0;
        this.lastInsightUpdate = 0;
        this.currentInsight = "Trip reset. Ready for live telemetry.";
    }

    /**
     * @brief Calculate instantaneous fuel consumption from MAF and Speed.
     * @param {number|null} mafGramsPerSec Mass air flow in g/s
     * @param {number|null} speedKmh Vehicle speed in km/h
     * @returns {{ value: string, unit: string }}
     */
    calculateFuelConsumption(mafGramsPerSec, speedKmh) {
        if (mafGramsPerSec === null || mafGramsPerSec === undefined || isNaN(mafGramsPerSec)) {
            return { value: '--', unit: 'L/100km' };
        }

        // Stoichiometric air-fuel ratio = 14.7 : 1
        // Gasoline density ~ 720 g/L
        // Fuel rate (L/h) = (MAF * 3600) / (14.7 * 720) = MAF * 0.34017
        const fuelLitersPerHour = mafGramsPerSec * 0.34017;

        if (speedKmh !== null && speedKmh >= 5.0) {
            // Standard L/100km = (Fuel L/h / Speed km/h) * 100
            const lPer100km = (fuelLitersPerHour / speedKmh) * 100.0;
            return {
                value: lPer100km.toFixed(1),
                unit: 'L/100km'
            };
        } else {
            // Idle consumption in Liters/hour
            return {
                value: fuelLitersPerHour.toFixed(1),
                unit: 'L/h'
            };
        }
    }

    /**
     * @brief Process real-time telemetry frame and evaluate driving behavior.
     */
    processTelemetry(packet) {
        const now = performance.now();
        const dt = (now - this.lastTimestamp) / 1000;
        this.lastTimestamp = now;

        const speed = packet.speed ?? 0;
        const rpm = packet.rpm ?? 0;
        const throttle = packet.throttle_position ?? 0;
        const coolant = packet.coolant_temperature;

        if (dt > 0.05 && dt < 2.0) {
            // Rate of speed change (acceleration / deceleration in m/s^2)
            const deltaSpeedKmh = speed - this.lastSpeed;
            const accelMps2 = (deltaSpeedKmh * (1000 / 3600)) / dt;

            // Rate of throttle change (% / sec)
            const deltaThrottle = Math.abs(throttle - this.lastThrottle) / dt;

            // Detect aggressive events
            if (accelMps2 > 3.2 && throttle > 50) {
                this.aggressiveAccelEvents++;
                this.accelScore = Math.max(40, this.accelScore - 1.5);
            } else if (accelMps2 < -3.5) {
                this.hardBrakingEvents++;
                this.accelScore = Math.max(45, this.accelScore - 1.0);
            } else if (accelMps2 > 0 && accelMps2 < 1.8) {
                // Smooth progressive acceleration recovery
                this.accelScore = Math.min(98, this.accelScore + 0.1);
            }

            if (deltaThrottle > 60.0) {
                this.throttleScore = Math.max(50, this.throttleScore - 1.0);
            } else if (throttle > 5.0 && deltaThrottle < 15.0) {
                this.throttleScore = Math.min(99, this.throttleScore + 0.08);
            }

            // High RPM penalty
            let rpmPenalty = 0;
            if (rpm > 4200) rpmPenalty = 15;
            else if (rpm > 3200) rpmPenalty = 6;

            // Overall Eco Score calculation
            const rawScore = (this.accelScore * 0.5) + (this.throttleScore * 0.5) - rpmPenalty;
            this.ecoScore = Math.round(Math.max(10, Math.min(99, rawScore)));

            this.lastSpeed = speed;
            this.lastThrottle = throttle;
        }

        // Generate context-aware AI speech insights every 3.5 seconds
        if (now - this.lastInsightUpdate > 3500) {
            this.lastInsightUpdate = now;
            this.currentInsight = this.generateInsight(speed, rpm, throttle, coolant);
        }
    }

    generateInsight(speed, rpm, throttle, coolant) {
        if (coolant !== null && coolant < 65) {
            return `Engine warmup phase (${Math.round(coolant)}°C). Keep RPM below 2,500 until oil reaches operating temperature.`;
        }

        if (speed === 0 && throttle === 0) {
            return "Engine idling at normal speed (800 RPM). Auxiliary electrical loads within normal range.";
        }

        if (throttle > 45 && speed < 60) {
            return "Heavy acceleration detected. Smooth out throttle application to gain up to +14% fuel efficiency.";
        }

        if (speed >= 60 && rpm <= 2300 && throttle < 25) {
            return "Excellent cruising efficiency. Powertrain operating in optimum thermal and brake-specific fuel zone.";
        }

        if (throttle === 0 && speed > 30) {
            return "Deceleration fuel cutoff active (0.0 L/h). Utilizing engine braking efficiently.";
        }

        if (this.ecoScore >= 88) {
            return `High Eco Score (${this.ecoScore}%). Driving style is smooth, aerodynamic, and fuel-optimal.`;
        }

        return "Active telemetry monitoring. Real-time ECU diagnostics nominal.";
    }
}

window.aiAnalytics = new AIAnalytics();
