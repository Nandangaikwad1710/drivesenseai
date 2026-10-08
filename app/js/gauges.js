/**
 * @file gauges.js
 * @brief High-DPI Canvas vector gauge rendering for Speedometer and Tachometer.
 */

class GaugeRenderer {
    constructor() {
        this.speedCanvas = document.getElementById('canvas-speedometer');
        this.rpmCanvas = document.getElementById('canvas-tachometer');

        this.currentSpeed = 0;
        this.targetSpeed = 0;
        this.currentRpm = 0;
        this.targetRpm = 0;

        this.speedUnit = 'kmh'; // 'kmh' or 'mph'
        this.maxSpeed = 240;
        this.maxRpm = 8000;

        this.setupCanvases();
        this.startRenderLoop();
    }

    setupCanvases() {
        const dpr = window.devicePixelRatio || 1;
        [this.speedCanvas, this.rpmCanvas].forEach(canvas => {
            if (!canvas) return;
            const rect = canvas.getBoundingClientRect();
            canvas.width = (rect.width || 340) * dpr;
            canvas.height = (rect.height || 340) * dpr;
            const ctx = canvas.getContext('2d');
            ctx.scale(dpr, dpr);
        });
    }

    setSpeed(speedVal) {
        if (speedVal === null || isNaN(speedVal)) {
            this.targetSpeed = 0;
        } else {
            let val = speedVal;
            if (this.speedUnit === 'mph') {
                val = val * 0.621371;
            }
            this.targetSpeed = Math.min(this.maxSpeed, Math.max(0, val));
        }
    }

    setRpm(rpmVal) {
        if (rpmVal === null || isNaN(rpmVal)) {
            this.targetRpm = 0;
        } else {
            this.targetRpm = Math.min(this.maxRpm, Math.max(0, rpmVal));
        }
    }

    setUnit(unit) {
        this.speedUnit = unit;
        this.maxSpeed = unit === 'mph' ? 160 : 240;
    }

    startRenderLoop() {
        const render = () => {
            // Smooth interpolation (spring-like easing)
            this.currentSpeed += (this.targetSpeed - this.currentSpeed) * 0.15;
            this.currentRpm += (this.targetRpm - this.currentRpm) * 0.15;

            this.drawSpeedometer();
            this.drawTachometer();

            requestAnimationFrame(render);
        };
        requestAnimationFrame(render);
    }

    drawSpeedometer() {
        if (!this.speedCanvas) return;
        const ctx = this.speedCanvas.getContext('2d');
        const dpr = window.devicePixelRatio || 1;
        const width = this.speedCanvas.width / dpr;
        const height = this.speedCanvas.height / dpr;
        const cx = width / 2;
        const cy = height / 2;
        const radius = width * 0.42;

        ctx.clearRect(0, 0, width, height);

        const startAngle = 0.75 * Math.PI; // 135 deg
        const endAngle = 2.25 * Math.PI;   // 405 deg (270 deg total)
        const totalAngle = endAngle - startAngle;

        // 1. Background Track Arc
        ctx.beginPath();
        ctx.arc(cx, cy, radius, startAngle, endAngle, false);
        ctx.strokeStyle = 'rgba(255, 255, 255, 0.08)';
        ctx.lineWidth = 14;
        ctx.lineCap = 'round';
        ctx.stroke();

        // 2. Active Speed Glow Arc
        const speedRatio = Math.min(1.0, this.currentSpeed / this.maxSpeed);
        const currentAngle = startAngle + (speedRatio * totalAngle);

        if (speedRatio > 0.005) {
            ctx.beginPath();
            ctx.arc(cx, cy, radius, startAngle, currentAngle, false);
            const gradient = ctx.createLinearGradient(0, height, width, 0);
            gradient.addColorStop(0, '#00d2ff');
            gradient.addColorStop(1, '#00f0ff');
            ctx.strokeStyle = gradient;
            ctx.lineWidth = 14;
            ctx.lineCap = 'round';
            ctx.shadowColor = 'rgba(0, 240, 255, 0.5)';
            ctx.shadowBlur = 18;
            ctx.stroke();
            ctx.shadowBlur = 0; // Reset
        }

        // 3. Ticks and Labels
        const majorStep = this.speedUnit === 'mph' ? 20 : 30;
        const numTicks = this.maxSpeed / majorStep;

        for (let i = 0; i <= numTicks; i++) {
            const val = i * majorStep;
            const angle = startAngle + (val / this.maxSpeed) * totalAngle;

            // Major Tick
            const innerR = radius - 18;
            const outerR = radius - 6;
            const x1 = cx + innerR * Math.cos(angle);
            const y1 = cy + innerR * Math.sin(angle);
            const x2 = cx + outerR * Math.cos(angle);
            const y2 = cy + outerR * Math.sin(angle);

            ctx.beginPath();
            ctx.moveTo(x1, y1);
            ctx.lineTo(x2, y2);
            ctx.strokeStyle = (val <= this.currentSpeed) ? 'rgba(0, 240, 255, 0.9)' : 'rgba(255, 255, 255, 0.3)';
            ctx.lineWidth = 2.5;
            ctx.stroke();

            // Label text
            const textR = radius - 34;
            const tx = cx + textR * Math.cos(angle);
            const ty = cy + textR * Math.sin(angle);

            ctx.font = '600 11px "JetBrains Mono", monospace';
            ctx.fillStyle = (val <= this.currentSpeed) ? '#ffffff' : 'rgba(255, 255, 255, 0.4)';
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText(val.toString(), tx, ty);
        }

        // 4. Sweeping Needle
        const needleAngle = startAngle + (speedRatio * totalAngle);
        const needleLength = radius - 15;
        const nx = cx + needleLength * Math.cos(needleAngle);
        const ny = cy + needleLength * Math.sin(needleAngle);

        ctx.save();
        ctx.beginPath();
        ctx.moveTo(cx, cy);
        ctx.lineTo(nx, ny);
        ctx.strokeStyle = '#00f0ff';
        ctx.lineWidth = 3.5;
        ctx.shadowColor = 'rgba(0, 240, 255, 0.8)';
        ctx.shadowBlur = 12;
        ctx.stroke();
        ctx.restore();

        // 5. Center Pivot Cap
        ctx.beginPath();
        ctx.arc(cx, cy, 14, 0, Math.PI * 2);
        ctx.fillStyle = '#101622';
        ctx.fill();
        ctx.lineWidth = 3;
        ctx.strokeStyle = '#00f0ff';
        ctx.stroke();

        ctx.beginPath();
        ctx.arc(cx, cy, 5, 0, Math.PI * 2);
        ctx.fillStyle = '#00f0ff';
        ctx.fill();
    }

    drawTachometer() {
        if (!this.rpmCanvas) return;
        const ctx = this.rpmCanvas.getContext('2d');
        const dpr = window.devicePixelRatio || 1;
        const width = this.rpmCanvas.width / dpr;
        const height = this.rpmCanvas.height / dpr;
        const cx = width / 2;
        const cy = height / 2;
        const radius = width * 0.42;

        ctx.clearRect(0, 0, width, height);

        const startAngle = 0.75 * Math.PI;
        const endAngle = 2.25 * Math.PI;
        const totalAngle = endAngle - startAngle;

        // 1. Background Arc with Redline Highlight
        ctx.beginPath();
        ctx.arc(cx, cy, radius, startAngle, endAngle, false);
        ctx.strokeStyle = 'rgba(255, 255, 255, 0.08)';
        ctx.lineWidth = 14;
        ctx.lineCap = 'round';
        ctx.stroke();

        // Redline background zone (6500 to 8000 RPM)
        const redlineStartAngle = startAngle + (6500 / this.maxRpm) * totalAngle;
        ctx.beginPath();
        ctx.arc(cx, cy, radius, redlineStartAngle, endAngle, false);
        ctx.strokeStyle = 'rgba(255, 42, 95, 0.3)';
        ctx.lineWidth = 14;
        ctx.stroke();

        // 2. Active RPM Glow Arc
        const rpmRatio = Math.min(1.0, this.currentRpm / this.maxRpm);
        const currentAngle = startAngle + (rpmRatio * totalAngle);

        if (rpmRatio > 0.005) {
            ctx.beginPath();
            ctx.arc(cx, cy, radius, startAngle, currentAngle, false);
            const isRedline = this.currentRpm >= 6500;
            const gradient = ctx.createLinearGradient(0, height, width, 0);
            if (isRedline) {
                gradient.addColorStop(0, '#ffaa00');
                gradient.addColorStop(1, '#ff2a5f');
                ctx.shadowColor = 'rgba(255, 42, 95, 0.8)';
            } else {
                gradient.addColorStop(0, '#00f0ff');
                gradient.addColorStop(1, '#ffaa00');
                ctx.shadowColor = 'rgba(255, 170, 0, 0.5)';
            }
            ctx.strokeStyle = gradient;
            ctx.lineWidth = 14;
            ctx.lineCap = 'round';
            ctx.shadowBlur = 18;
            ctx.stroke();
            ctx.shadowBlur = 0;
        }

        // 3. Ticks and Labels (0 - 8 representing x1000 RPM)
        for (let i = 0; i <= 8; i++) {
            const val = i * 1000;
            const angle = startAngle + (val / this.maxRpm) * totalAngle;
            const isRedZone = val >= 6500;

            const innerR = radius - 18;
            const outerR = radius - 6;
            const x1 = cx + innerR * Math.cos(angle);
            const y1 = cy + innerR * Math.sin(angle);
            const x2 = cx + outerR * Math.cos(angle);
            const y2 = cy + outerR * Math.sin(angle);

            ctx.beginPath();
            ctx.moveTo(x1, y1);
            ctx.lineTo(x2, y2);
            ctx.strokeStyle = isRedZone ? '#ff2a5f' : ((val <= this.currentRpm) ? '#ffaa00' : 'rgba(255, 255, 255, 0.3)');
            ctx.lineWidth = 2.5;
            ctx.stroke();

            // Label text (0, 1, 2, ... 8)
            const textR = radius - 34;
            const tx = cx + textR * Math.cos(angle);
            const ty = cy + textR * Math.sin(angle);

            ctx.font = '700 12px "JetBrains Mono", monospace';
            ctx.fillStyle = isRedZone ? '#ff2a5f' : ((val <= this.currentRpm) ? '#ffffff' : 'rgba(255, 255, 255, 0.4)');
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText(i.toString(), tx, ty);
        }

        // 4. Sweeping Needle
        const needleAngle = startAngle + (rpmRatio * totalAngle);
        const needleLength = radius - 15;
        const nx = cx + needleLength * Math.cos(needleAngle);
        const ny = cy + needleLength * Math.sin(needleAngle);

        const needleColor = this.currentRpm >= 6500 ? '#ff2a5f' : '#ffaa00';

        ctx.save();
        ctx.beginPath();
        ctx.moveTo(cx, cy);
        ctx.lineTo(nx, ny);
        ctx.strokeStyle = needleColor;
        ctx.lineWidth = 3.5;
        ctx.shadowColor = needleColor;
        ctx.shadowBlur = 14;
        ctx.stroke();
        ctx.restore();

        // 5. Center Pivot Cap
        ctx.beginPath();
        ctx.arc(cx, cy, 14, 0, Math.PI * 2);
        ctx.fillStyle = '#101622';
        ctx.fill();
        ctx.lineWidth = 3;
        ctx.strokeStyle = needleColor;
        ctx.stroke();

        ctx.beginPath();
        ctx.arc(cx, cy, 5, 0, Math.PI * 2);
        ctx.fillStyle = needleColor;
        ctx.fill();
    }
}

window.gaugeRenderer = new GaugeRenderer();
