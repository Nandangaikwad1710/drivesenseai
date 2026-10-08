/**
 * @file virtual-esp32.js
 * @brief Built-in software twin of the ESP32 DriveSense OBD firmware.
 *        Allows full testing of dials, charts, and trip analytics in any browser.
 */

class VirtualESP32 {
    constructor() {
        this.isActive = false;
        this.isStreaming = false;
        this.timer = null;
        this.sampleIntervalMs = 200; // 5Hz stream matching ESP32 firmware

        // Simulation State
        this.phase = 'IDLE';
        this.phaseStartMs = 0;
        this.speed = 0.0;
        this.rpm = 800.0;
        this.throttle = 0.0;
        this.engineLoad = 18.0;
        this.coolant = 48.0;
        this.iat = 28.0;
        this.maf = 2.8;
        this.fuel = 68.5;

        this.onTelemetryCallback = null;
        this.onStatusCallback = null;
        this.onLogCallback = null;
        this.onConnectionChangeCallback = null;
    }

    start() {
        this.isActive = true;
        this.isStreaming = true;
        this.phase = 'IDLE';
        this.phaseStartMs = performance.now();
        this.log('[SIM] Virtual ESP32 Started. Generating driving cycle at 5Hz (200ms)...', 'sys');
        
        if (this.onConnectionChangeCallback) {
            this.onConnectionChangeCallback(true);
        }

        this.timer = setInterval(() => this.step(), this.sampleIntervalMs);
    }

    stop() {
        this.isActive = false;
        this.isStreaming = false;
        if (this.timer) {
            clearInterval(this.timer);
            this.timer = null;
        }
        this.log('[SIM] Virtual ESP32 Stopped.', 'warn');
        if (this.onConnectionChangeCallback) {
            this.onConnectionChangeCallback(false);
        }
    }

    log(msg, type = 'sys') {
        if (this.onLogCallback) {
            this.onLogCallback(msg, type);
        }
    }

    sendCommand(cmdStr) {
        this.log(`[SIM] TX -> ${cmdStr}`, 'tx');

        setTimeout(() => {
            if (cmdStr.includes('START_TELEMETRY')) {
                this.isStreaming = true;
                this.respondStatus('{"status":"TELEMETRY_STARTED","rate_ms":200}');
            } else if (cmdStr.includes('STOP_TELEMETRY')) {
                this.isStreaming = false;
                this.respondStatus('{"status":"TELEMETRY_STOPPED"}');
            } else if (cmdStr.includes('GET_DEVICE_INFO')) {
                this.respondStatus(JSON.stringify({
                    device_name: "DriveSense OBD (Virtual)",
                    firmware_version: "1.0.0",
                    hardware_version: "1.0-SIM",
                    bluetooth_status: "CONNECTED",
                    obd_status: "CONNECTED"
                }));
            } else if (cmdStr.includes('GET_SUPPORTED_PIDS')) {
                this.respondStatus(JSON.stringify({
                    supported_mask_01_20: "0xBE1FA813",
                    supported_mask_21_40: "0x80002001"
                }));
            } else if (cmdStr.includes('GET_CONNECTION_STATUS')) {
                this.respondStatus(JSON.stringify({
                    bluetooth: "CONNECTED",
                    obd: "CONNECTED",
                    telemetry: this.isStreaming ? "ACTIVE" : "STOPPED",
                    simulation: true
                }));
            } else if (cmdStr.includes('SET_SIMULATION_MODE')) {
                this.respondStatus('{"simulation_mode":true}');
            }
        }, 80);
    }

    respondStatus(jsonStr) {
        this.log(`[SIM] RX Status -> ${jsonStr}`, 'rx');
        if (this.onStatusCallback) {
            this.onStatusCallback(jsonStr);
        }
    }

    step() {
        if (!this.isActive || !this.isStreaming) return;

        const now = performance.now();
        let elapsed = now - this.phaseStartMs;

        // Phase Transitions matching ESP32 firmware TelemetryManager
        if (this.phase === 'IDLE' && elapsed >= 4000) {
            this.phase = 'ACCELERATING';
            this.phaseStartMs = now;
            elapsed = 0;
            this.log('[SIM] Driving Cycle Transition: Idle -> Accelerating', 'obd');
        } else if (this.phase === 'ACCELERATING' && elapsed >= 7000) {
            this.phase = 'CRUISING';
            this.phaseStartMs = now;
            elapsed = 0;
            this.log('[SIM] Driving Cycle Transition: Accelerating -> Cruising', 'obd');
        } else if (this.phase === 'CRUISING' && elapsed >= 8000) {
            this.phase = 'DECELERATING';
            this.phaseStartMs = now;
            elapsed = 0;
            this.log('[SIM] Driving Cycle Transition: Cruising -> Decelerating', 'obd');
        } else if (this.phase === 'DECELERATING' && elapsed >= 5000) {
            this.phase = 'IDLE';
            this.phaseStartMs = now;
            elapsed = 0;
            this.log('[SIM] Driving Cycle Transition: Decelerating -> Idle', 'obd');
        }

        // Physics Calculation
        switch (this.phase) {
            case 'IDLE':
                this.throttle = 0.0;
                this.rpm = 800.0 + 20.0 * Math.sin(now / 300.0);
                this.speed = 0.0;
                this.engineLoad = 18.0 + 2.0 * Math.sin(now / 500.0);
                this.maf = 2.8 + 0.2 * Math.sin(now / 400.0);
                break;

            case 'ACCELERATING': {
                let progress = elapsed / 7000.0;
                if (progress > 1.0) progress = 1.0;
                this.throttle = 35.0 + 12.0 * progress;
                this.speed = progress * 68.0; // 0 -> 68 km/h
                this.rpm = 800.0 + (progress * 2600.0); // 800 -> 3400 RPM
                this.engineLoad = 50.0 + (progress * 25.0);
                this.maf = 3.0 + (progress * 28.0);
                break;
            }

            case 'CRUISING':
                this.throttle = 18.0 + 3.0 * Math.sin(now / 800.0);
                this.rpm = 2050.0 + 80.0 * Math.sin(now / 600.0);
                this.speed = 65.0 + 5.0 * Math.sin(now / 1200.0);
                this.engineLoad = 38.0 + 4.0 * Math.sin(now / 700.0);
                this.maf = 10.5 + 1.5 * Math.sin(now / 600.0);
                break;

            case 'DECELERATING': {
                let progress = elapsed / 5000.0;
                if (progress > 1.0) progress = 1.0;
                this.throttle = 0.0;
                this.speed = 65.0 * (1.0 - progress);
                this.rpm = 2000.0 - (progress * 1200.0);
                this.engineLoad = 20.0 - (progress * 5.0);
                this.maf = 8.0 - (progress * 5.2);
                break;
            }
        }

        // Coolant temp slowly warms up to 90C
        if (this.coolant < 90.0) {
            this.coolant += 0.04;
        }

        // Ambient IAT
        this.iat = 28.0 + 0.8 * Math.sin(now / 4000.0);

        // Fuel level slowly decreases
        if (this.fuel > 10.0) {
            this.fuel -= 0.001;
        }

        const packet = {
            timestamp: Math.floor(Date.now() / 1000),
            speed: parseFloat(this.speed.toFixed(1)),
            rpm: Math.round(this.rpm),
            engine_load: parseFloat(this.engineLoad.toFixed(1)),
            throttle_position: parseFloat(this.throttle.toFixed(1)),
            coolant_temperature: parseFloat(this.coolant.toFixed(1)),
            intake_air_temperature: parseFloat(this.iat.toFixed(1)),
            mass_air_flow: parseFloat(this.maf.toFixed(2)),
            fuel_level: parseFloat(this.fuel.toFixed(1))
        };

        const jsonStr = JSON.stringify(packet);
        if (this.onTelemetryCallback) {
            this.onTelemetryCallback(packet, jsonStr);
        }
    }
}

window.virtualESP32 = new VirtualESP32();
