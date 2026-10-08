/**
 * @file charts.js
 * @brief High-performance canvas-based multi-trace telemetry strip chart.
 */

class TelemetryChart {
    constructor() {
        this.canvas = document.getElementById('canvas-stripchart');
        this.setupCanvas();
        window.addEventListener('resize', () => this.setupCanvas());
    }

    setupCanvas() {
        if (!this.canvas) return;
        const dpr = window.devicePixelRatio || 1;
        const rect = this.canvas.getBoundingClientRect();
        this.canvas.width = (rect.width || 800) * dpr;
        this.canvas.height = (rect.height || 180) * dpr;
        const ctx = this.canvas.getContext('2d');
        ctx.scale(dpr, dpr);
    }

    render(history) {
        if (!this.canvas || !history || history.length < 2) return;

        const ctx = this.canvas.getContext('2d');
        const dpr = window.devicePixelRatio || 1;
        const width = this.canvas.width / dpr;
        const height = this.canvas.height / dpr;

        ctx.clearRect(0, 0, width, height);

        const padTop = 15;
        const padBottom = 25;
        const chartH = height - padTop - padBottom;
        const maxVal = 160; // Max scale for Speed & Normalized RPM

        // Draw horizontal grid lines
        ctx.strokeStyle = 'rgba(255, 255, 255, 0.05)';
        ctx.lineWidth = 1;
        for (let i = 0; i <= 4; i++) {
            const y = padTop + (chartH * (i / 4));
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(width, y);
            ctx.stroke();

            // Label
            const val = Math.round(maxVal * (1 - i / 4));
            ctx.font = '500 10px "JetBrains Mono", monospace';
            ctx.fillStyle = 'rgba(255, 255, 255, 0.25)';
            ctx.fillText(val.toString(), 6, y - 4);
        }

        const count = history.length;
        const stepX = width / (count - 1);

        // Helper to plot a series
        const plotSeries = (extractFn, color, glowColor, fillGradient = false) => {
            ctx.beginPath();
            let first = true;

            for (let i = 0; i < count; i++) {
                const val = extractFn(history[i]);
                if (val === null || val === undefined) continue;

                const x = i * stepX;
                const clamped = Math.max(0, Math.min(maxVal, val));
                const y = padTop + chartH * (1 - (clamped / maxVal));

                if (first) {
                    ctx.moveTo(x, y);
                    first = false;
                } else {
                    ctx.lineTo(x, y);
                }
            }

            ctx.save();
            ctx.strokeStyle = color;
            ctx.lineWidth = 2;
            ctx.shadowColor = glowColor;
            ctx.shadowBlur = 8;
            ctx.stroke();

            if (fillGradient && !first) {
                ctx.lineTo(width, height - padBottom);
                ctx.lineTo(0, height - padBottom);
                ctx.closePath();
                const grad = ctx.createLinearGradient(0, padTop, 0, height);
                grad.addColorStop(0, color.replace(')', ', 0.15)').replace('rgb', 'rgba'));
                grad.addColorStop(1, 'rgba(0, 0, 0, 0)');
                ctx.fillStyle = grad;
                ctx.fill();
            }
            ctx.restore();
        };

        // 1. Engine Load (Green)
        plotSeries(d => d.load, '#00ff9d', 'rgba(0, 255, 157, 0.4)');

        // 2. Throttle (Amber)
        plotSeries(d => d.throttle, '#ffb800', 'rgba(255, 184, 0, 0.4)');

        // 3. RPM / 50 (Red) - 8000 RPM maps to 160 on scale
        plotSeries(d => d.rpm !== null ? d.rpm / 50 : null, '#ff2a5f', 'rgba(255, 42, 95, 0.4)');

        // 4. Speed (Cyan, with gradient fill)
        plotSeries(d => d.speed, '#00f0ff', 'rgba(0, 240, 255, 0.6)', true);
    }
}

window.telemetryChart = new TelemetryChart();
