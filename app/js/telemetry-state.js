/**
 * @file telemetry-state.js
 * @brief Telemetry state manager, trip distance integration, and rate tracking.
 */

class TelemetryState {
    constructor() {
        this.reset();
    }

    reset() {
        this.latest = {
            timestamp: 0,
            speed: null,
            rpm: null,
            engine_load: null,
            throttle_position: null,
            coolant_temperature: null,
            intake_air_temperature: null,
            mass_air_flow: null,
            fuel_level: null
        };

        this.trip = {
            startTime: null,
            distanceKm: 0.0,
            peakSpeed: 0.0,
            peakRpm: 0.0,
            avgSpeed: 0.0,
            speedSamples: [],
            lastUpdateTime: null
        };

        this.rateTracker = {
            packetCount: 0,
            lastSecond: performance.now(),
            currentHz: 0
        };

        this.history = [];
        this.maxHistory = 300; // 60 seconds at 5Hz
    }

    update(packet) {
        const now = performance.now();

        if (!this.trip.startTime) {
            this.trip.startTime = now;
            this.trip.lastUpdateTime = now;
        }

        // Calculate delta time for distance integration
        const dtSec = (now - this.trip.lastUpdateTime) / 1000;
        this.trip.lastUpdateTime = now;

        // Packet arrival frequency calculation
        this.rateTracker.packetCount++;
        if (now - this.rateTracker.lastSecond >= 1000) {
            this.rateTracker.currentHz = this.rateTracker.packetCount;
            this.rateTracker.packetCount = 0;
            this.rateTracker.lastSecond = now;
        }

        // Merge incoming fields
        this.latest = { ...this.latest, ...packet };

        // Distance & speed metrics
        if (packet.speed !== null && packet.speed !== undefined) {
            const speedKmh = packet.speed;
            if (speedKmh > this.trip.peakSpeed) {
                this.trip.peakSpeed = speedKmh;
            }

            // Distance integration: km = (km/h) * (seconds / 3600)
            if (dtSec > 0 && dtSec < 5.0) {
                this.trip.distanceKm += speedKmh * (dtSec / 3600);
            }

            this.trip.speedSamples.push(speedKmh);
            if (this.trip.speedSamples.length > 500) {
                this.trip.speedSamples.shift();
            }

            const sum = this.trip.speedSamples.reduce((a, b) => a + b, 0);
            this.trip.avgSpeed = Math.round(sum / this.trip.speedSamples.length);
        }

        if (packet.rpm !== null && packet.rpm !== undefined) {
            if (packet.rpm > this.trip.peakRpm) {
                this.trip.peakRpm = packet.rpm;
            }
        }

        // Add to sliding history buffer
        this.history.push({
            time: now,
            speed: packet.speed,
            rpm: packet.rpm,
            throttle: packet.throttle_position,
            load: packet.engine_load
        });

        if (this.history.length > this.maxHistory) {
            this.history.shift();
        }
    }

    getTripDurationFormatted() {
        if (!this.trip.startTime) return "00:00:00";
        const elapsedSec = Math.floor((performance.now() - this.trip.startTime) / 1000);
        const hrs = Math.floor(elapsedSec / 3600).toString().padStart(2, '0');
        const mins = Math.floor((elapsedSec % 3600) / 60).toString().padStart(2, '0');
        const secs = (elapsedSec % 60).toString().padStart(2, '0');
        return `${hrs}:${mins}:${secs}`;
    }
}

window.telemetryState = new TelemetryState();
