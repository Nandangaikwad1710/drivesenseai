/**
 * @file app.js
 * @brief Main UI controller and application orchestrator for DriveSense AI Cockpit.
 */

document.addEventListener('DOMContentLoaded', () => {
    // UI Elements
    const btnConnect = document.getElementById('btn-connect');
    const btnConnectLabel = document.getElementById('btn-connect-label');
    const btnToggleSim = document.getElementById('btn-toggle-sim');
    const simStateText = document.getElementById('sim-state-text');
    const connectionDot = document.getElementById('connection-dot');
    const connectionStatusText = document.getElementById('connection-status-text');
    const telemetryRateBadge = document.getElementById('telemetry-rate-badge');

    // Action Buttons
    const btnStartStream = document.getElementById('btn-stream-start');
    const btnStopStream = document.getElementById('btn-stream-stop');
    const btnDeviceInfo = document.getElementById('btn-device-info');

    // HUD Elements
    const hudTripTime = document.getElementById('hud-trip-time');
    const hudTripDistance = document.getElementById('hud-trip-distance');
    const hudAvgSpeed = document.getElementById('hud-avg-speed');
    const hudPeakSpeed = document.getElementById('hud-peak-speed');
    const hudFuelRate = document.getElementById('hud-fuel-rate');
    const ecoScoreNumber = document.getElementById('eco-score-number');
    const ecoProgressArc = document.getElementById('eco-progress-arc');
    const ecoStatusText = document.getElementById('eco-status-text');

    // Gauge Elements
    const digitalSpeed = document.getElementById('digital-speed');
    const digitalSpeedUnit = document.getElementById('digital-speed-unit');
    const digitalRpm = document.getElementById('digital-rpm');
    const unitToggle = document.getElementById('unit-toggle');

    // AI Insight Elements
    const aiInsightText = document.getElementById('ai-insight-text');
    const scoreAccelVal = document.getElementById('score-accel-val');
    const scoreAccelBar = document.getElementById('score-accel-bar');
    const scoreThrottleVal = document.getElementById('score-throttle-val');
    const scoreThrottleBar = document.getElementById('score-throttle-bar');

    // Parameter Cards
    const valThrottle = document.getElementById('val-throttle');
    const barThrottle = document.getElementById('bar-throttle');
    const throttleSupport = document.getElementById('throttle-support');

    const valLoad = document.getElementById('val-load');
    const barLoad = document.getElementById('bar-load');
    const loadSupport = document.getElementById('load-support');

    const valCoolant = document.getElementById('val-coolant');
    const barCoolant = document.getElementById('bar-coolant');
    const coolantSupport = document.getElementById('coolant-support');
    const coolantStatusLabel = document.getElementById('coolant-status-label');

    const valIat = document.getElementById('val-iat');
    const barIat = document.getElementById('bar-iat');
    const iatSupport = document.getElementById('iat-support');

    const valMaf = document.getElementById('val-maf');
    const barMaf = document.getElementById('bar-maf');
    const mafSupport = document.getElementById('maf-support');

    const valFuel = document.getElementById('val-fuel');
    const barFuel = document.getElementById('bar-fuel');
    const fuelSupport = document.getElementById('fuel-support');

    // Terminal Drawer Elements
    const btnOpenTerminal = document.getElementById('btn-open-terminal');
    const terminalDrawer = document.getElementById('terminal-drawer');
    const btnCloseTerm = document.getElementById('btn-close-term');
    const btnClearTerm = document.getElementById('btn-clear-term');
    const terminalOutput = document.getElementById('terminal-output');

    // Raw JSON Modal
    const jsonModal = document.getElementById('json-modal');
    const btnCloseJson = document.getElementById('btn-close-json');
    const rawJsonDisplay = document.getElementById('raw-json-display');

    let currentUnit = 'kmh';
    let isVirtualActive = false;
    let rawJsonPacket = null;

    // ==========================================
    // Logging Helper
    // ==========================================
    function logToTerminal(msg, type = 'sys') {
        if (!terminalOutput) return;
        const line = document.createElement('div');
        line.className = `term-line ${type}`;
        const timeStr = new Date().toLocaleTimeString();
        line.textContent = `[${timeStr}] ${msg}`;
        terminalOutput.appendChild(line);
        terminalOutput.scrollTop = terminalOutput.scrollHeight;
    }

    // Connect logging hooks
    window.bleService.onLogCallback = logToTerminal;
    window.virtualESP32.onLogCallback = logToTerminal;

    // ==========================================
    // Connection Management (BLE & Virtual)
    // ==========================================
    function updateConnectionUI(connected, isStreaming = false) {
        if (connected) {
            connectionDot.className = isStreaming ? 'pulse-dot streaming' : 'pulse-dot connected';
            connectionStatusText.textContent = isStreaming ? 'STREAMING' : 'CONNECTED';
            connectionStatusText.className = 'status-label active';
            btnConnect.classList.add('connected');
            btnConnectLabel.textContent = isVirtualActive ? 'Disconnect Sim' : 'Disconnect BLE';
            btnStartStream.disabled = isStreaming;
            btnStopStream.disabled = !isStreaming;
        } else {
            connectionDot.className = 'pulse-dot';
            connectionStatusText.textContent = 'DISCONNECTED';
            connectionStatusText.className = 'status-label';
            btnConnect.classList.remove('connected');
            btnConnectLabel.textContent = 'Connect Device';
            btnStartStream.disabled = true;
            btnStopStream.disabled = true;
            telemetryRateBadge.textContent = '0 Hz';
        }
    }

    // BLE Service Callbacks
    window.bleService.onConnectionChangeCallback = (connected) => {
        updateConnectionUI(connected, connected);
    };

    window.virtualESP32.onConnectionChangeCallback = (connected) => {
        updateConnectionUI(connected, connected);
    };

    // Telemetry Consumer
    function onIncomingTelemetry(packet, rawJson) {
        rawJsonPacket = rawJson;
        window.telemetryState.update(packet);
        window.aiAnalytics.processTelemetry(packet);

        // 1. Update Gauges
        window.gaugeRenderer.setSpeed(packet.speed);
        window.gaugeRenderer.setRpm(packet.rpm);

        // Digital Speed
        if (packet.speed !== null && !isNaN(packet.speed)) {
            let speedDisplay = packet.speed;
            if (currentUnit === 'mph') speedDisplay = speedDisplay * 0.621371;
            digitalSpeed.textContent = Math.round(speedDisplay);
        } else {
            digitalSpeed.textContent = '--';
        }

        // Digital RPM
        if (packet.rpm !== null && !isNaN(packet.rpm)) {
            digitalRpm.textContent = Math.round(packet.rpm);
        } else {
            digitalRpm.textContent = '--';
        }

        // 2. Auxiliary Parameter Cards & Missing Data Handling
        updateParamCard(valThrottle, barThrottle, throttleSupport, packet.throttle_position, '%', 100);
        updateParamCard(valLoad, barLoad, loadSupport, packet.engine_load, '%', 100);
        updateParamCard(valCoolant, barCoolant, coolantSupport, packet.coolant_temperature, '°C', 130);
        updateParamCard(valIat, barIat, iatSupport, packet.intake_air_temperature, '°C', 80);
        updateParamCard(valMaf, barMaf, mafSupport, packet.mass_air_flow, 'g/s', 60);
        updateParamCard(valFuel, barFuel, fuelSupport, packet.fuel_level, '%', 100);

        // Coolant Status
        if (packet.coolant_temperature !== null) {
            if (packet.coolant_temperature < 70) coolantStatusLabel.textContent = "Engine Warmup";
            else if (packet.coolant_temperature <= 100) coolantStatusLabel.textContent = "Nominal 90°C";
            else coolantStatusLabel.textContent = "High Temp Warning!";
        }

        // 3. Update HUD Strip
        hudTripTime.textContent = window.telemetryState.getTripDurationFormatted();
        hudTripDistance.textContent = window.telemetryState.trip.distanceKm.toFixed(1);
        hudAvgSpeed.textContent = window.telemetryState.trip.avgSpeed;
        hudPeakSpeed.textContent = Math.round(window.telemetryState.trip.peakSpeed);

        // Fuel Economy
        const fuelEcon = window.aiAnalytics.calculateFuelConsumption(packet.mass_air_flow, packet.speed);
        hudFuelRate.textContent = fuelEcon.value;

        // Eco Score
        const ecoVal = window.aiAnalytics.ecoScore;
        ecoScoreNumber.textContent = ecoVal;
        ecoProgressArc.setAttribute('stroke-dasharray', `${ecoVal}, 100`);
        ecoStatusText.textContent = ecoVal >= 85 ? "Optimal" : (ecoVal >= 70 ? "Moderate" : "Aggressive");

        // 4. Update AI Insights Card
        aiInsightText.textContent = window.aiAnalytics.currentInsight;
        scoreAccelVal.textContent = `${Math.round(window.aiAnalytics.accelScore)}%`;
        scoreAccelBar.style.width = `${Math.round(window.aiAnalytics.accelScore)}%`;
        scoreThrottleVal.textContent = `${Math.round(window.aiAnalytics.throttleScore)}%`;
        scoreThrottleBar.style.width = `${Math.round(window.aiAnalytics.throttleScore)}%`;

        // 5. Update Telemetry Frequency Badge
        telemetryRateBadge.textContent = `${window.telemetryState.rateTracker.currentHz} Hz`;

        // 6. Update Strip Chart
        window.telemetryChart.render(window.telemetryState.history);
    }

    function updateParamCard(valEl, barEl, supportBadge, value, unit, maxVal) {
        if (value === null || value === undefined || isNaN(value)) {
            valEl.textContent = "N/A";
            valEl.style.color = "var(--text-muted)";
            if (barEl) barEl.style.width = "0%";
            if (supportBadge) {
                supportBadge.textContent = "UNSUPPORTED";
                supportBadge.className = "support-badge unsupported";
            }
        } else {
            valEl.textContent = (typeof value === 'number' && !Number.isInteger(value)) ? value.toFixed(1) : value;
            valEl.style.color = "#ffffff";
            if (barEl) {
                const pct = Math.min(100, Math.max(0, (value / maxVal) * 100));
                barEl.style.width = `${pct}%`;
            }
            if (supportBadge) {
                supportBadge.textContent = "SUPPORTED";
                supportBadge.className = "support-badge supported";
            }
        }
    }

    window.bleService.onTelemetryCallback = onIncomingTelemetry;
    window.virtualESP32.onTelemetryCallback = onIncomingTelemetry;

    // ==========================================
    // Event Handlers
    // ==========================================

    // Connect Button
    btnConnect.addEventListener('click', async () => {
        if (isVirtualActive) {
            window.virtualESP32.stop();
            isVirtualActive = false;
            simStateText.textContent = 'OFF';
            btnToggleSim.classList.remove('active');
            updateConnectionUI(false);
            return;
        }

        if (window.bleService.isConnected) {
            window.bleService.disconnect();
        } else {
            try {
                updateConnectionUI(false);
                connectionDot.className = 'pulse-dot connecting';
                connectionStatusText.textContent = 'CONNECTING...';
                await window.bleService.connect();
            } catch (err) {
                logToTerminal(`Connect error: ${err.message}`, 'err');
                updateConnectionUI(false);
            }
        }
    });

    // Virtual Simulator Toggle
    btnToggleSim.addEventListener('click', () => {
        if (window.bleService.isConnected) {
            window.bleService.disconnect();
        }

        isVirtualActive = !isVirtualActive;
        if (isVirtualActive) {
            simStateText.textContent = 'ON';
            btnToggleSim.classList.add('active');
            window.virtualESP32.start();
        } else {
            simStateText.textContent = 'OFF';
            btnToggleSim.classList.remove('active');
            window.virtualESP32.stop();
        }
    });

    // Unit Toggle
    unitToggle.addEventListener('click', () => {
        const opts = unitToggle.querySelectorAll('.unit-opt');
        if (currentUnit === 'kmh') {
            currentUnit = 'mph';
            opts[0].classList.remove('active');
            opts[1].classList.add('active');
            digitalSpeedUnit.textContent = 'MPH';
        } else {
            currentUnit = 'kmh';
            opts[1].classList.remove('active');
            opts[0].classList.add('active');
            digitalSpeedUnit.textContent = 'KM/H';
        }
        window.gaugeRenderer.setUnit(currentUnit);
    });

    // Stream Controls
    btnStartStream.addEventListener('click', () => {
        if (isVirtualActive) {
            window.virtualESP32.sendCommand('START_TELEMETRY');
            updateConnectionUI(true, true);
        } else if (window.bleService.isConnected) {
            window.bleService.sendCommand('START_TELEMETRY');
            updateConnectionUI(true, true);
        }
    });

    btnStopStream.addEventListener('click', () => {
        if (isVirtualActive) {
            window.virtualESP32.sendCommand('STOP_TELEMETRY');
            updateConnectionUI(true, false);
        } else if (window.bleService.isConnected) {
            window.bleService.sendCommand('STOP_TELEMETRY');
            updateConnectionUI(true, false);
        }
    });

    btnDeviceInfo.addEventListener('click', () => {
        if (isVirtualActive) {
            window.virtualESP32.sendCommand('GET_DEVICE_INFO');
        } else if (window.bleService.isConnected) {
            window.bleService.sendCommand('GET_DEVICE_INFO');
        } else {
            logToTerminal('Please connect to a device or enable Virtual ESP32 first.', 'warn');
        }
    });

    // Terminal Commands
    btnOpenTerminal.addEventListener('click', () => {
        terminalDrawer.classList.toggle('open');
    });

    btnCloseTerm.addEventListener('click', () => {
        terminalDrawer.classList.remove('open');
    });

    btnClearTerm.addEventListener('click', () => {
        terminalOutput.innerHTML = '';
    });

    document.querySelectorAll('.btn-cmd').forEach(btn => {
        btn.addEventListener('click', () => {
            const cmd = btn.getAttribute('data-cmd');
            if (isVirtualActive) {
                window.virtualESP32.sendCommand(cmd);
            } else if (window.bleService.isConnected) {
                window.bleService.sendCommand(cmd);
            } else {
                logToTerminal(`Cannot send ${cmd}: Not connected.`, 'warn');
            }
        });
    });

    // Raw JSON modal inspect (click telemetry rate badge)
    telemetryRateBadge.addEventListener('click', () => {
        if (rawJsonPacket) {
            try {
                const parsed = JSON.parse(rawJsonPacket);
                rawJsonDisplay.textContent = JSON.stringify(parsed, null, 2);
            } catch (e) {
                rawJsonDisplay.textContent = rawJsonPacket;
            }
        }
        jsonModal.classList.add('open');
    });

    btnCloseJson.addEventListener('click', () => {
        jsonModal.classList.remove('open');
    });
});
