/**
 * Simple Chart Library for Packet Rate Visualization
 */
class SimpleChart {
    constructor(canvasId, options = {}) {
        this.canvas = document.getElementById(canvasId);
        this.ctx = this.canvas.getContext('2d');
        this.options = {
            maxDataPoints: options.maxDataPoints || 60,
            lineWidth: options.lineWidth || 2,
            colors: options.colors || {
                rx: '#3b82f6',
                tx: '#10b981'
            },
            gridColor: options.gridColor || 'rgba(0, 0, 0, 0.1)',
            textColor: options.textColor || '#6b7280',
            padding: options.padding || 40
        };
        
        this.data = {
            rx: [],
            tx: []
        };
        
        this.maxValue = 10;
        this.resizeCanvas();
        window.addEventListener('resize', () => this.resizeCanvas());
    }
    
    resizeCanvas() {
        const rect = this.canvas.getBoundingClientRect();
        this.canvas.width = rect.width * window.devicePixelRatio;
        this.canvas.height = rect.height * window.devicePixelRatio;
        this.ctx.scale(window.devicePixelRatio, window.devicePixelRatio);
        this.width = rect.width;
        this.height = rect.height;
        this.draw();
    }
    
    addData(rxValue, txValue) {
        this.data.rx.push(rxValue);
        this.data.tx.push(txValue);
        
        if (this.data.rx.length > this.options.maxDataPoints) {
            this.data.rx.shift();
            this.data.tx.shift();
        }
        
        // Update max value for scaling
        const allValues = [...this.data.rx, ...this.data.tx];
        const currentMax = Math.max(...allValues, 10);
        this.maxValue = Math.ceil(currentMax * 1.2);
        
        this.draw();
    }
    
    draw() {
        const { padding } = this.options;
        const chartWidth = this.width - padding * 2;
        const chartHeight = this.height - padding * 2;
        
        // Clear canvas
        this.ctx.clearRect(0, 0, this.width, this.height);
        
        // Draw grid
        this.drawGrid(padding, chartWidth, chartHeight);
        
        // Draw lines
        this.drawLine(this.data.rx, this.options.colors.rx, padding, chartWidth, chartHeight);
        this.drawLine(this.data.tx, this.options.colors.tx, padding, chartWidth, chartHeight);
    }
    
    drawGrid(padding, chartWidth, chartHeight) {
        const { gridColor, textColor } = this.options;
        
        this.ctx.strokeStyle = gridColor;
        this.ctx.lineWidth = 1;
        this.ctx.font = '11px sans-serif';
        this.ctx.fillStyle = textColor;
        
        // Horizontal grid lines
        const gridLines = 5;
        for (let i = 0; i <= gridLines; i++) {
            const y = padding + (chartHeight / gridLines) * i;
            const value = Math.round(this.maxValue * (1 - i / gridLines));
            
            this.ctx.beginPath();
            this.ctx.moveTo(padding, y);
            this.ctx.lineTo(padding + chartWidth, y);
            this.ctx.stroke();
            
            // Y-axis labels
            this.ctx.fillText(value.toString(), 5, y + 4);
        }
        
        // Vertical grid lines
        const timeLabels = 6;
        for (let i = 0; i <= timeLabels; i++) {
            const x = padding + (chartWidth / timeLabels) * i;
            
            this.ctx.beginPath();
            this.ctx.moveTo(x, padding);
            this.ctx.lineTo(x, padding + chartHeight);
            this.ctx.stroke();
        }
    }
    
    drawLine(data, color, padding, chartWidth, chartHeight) {
        if (data.length < 2) return;
        
        this.ctx.strokeStyle = color;
        this.ctx.lineWidth = this.options.lineWidth;
        this.ctx.lineJoin = 'round';
        this.ctx.lineCap = 'round';
        
        this.ctx.beginPath();
        
        const stepX = chartWidth / (this.options.maxDataPoints - 1);
        const startIndex = Math.max(0, this.options.maxDataPoints - data.length);
        
        data.forEach((value, index) => {
            const x = padding + (startIndex + index) * stepX;
            const y = padding + chartHeight - (value / this.maxValue) * chartHeight;
            
            if (index === 0) {
                this.ctx.moveTo(x, y);
            } else {
                this.ctx.lineTo(x, y);
            }
        });
        
        this.ctx.stroke();
        
        // Draw fill
        this.ctx.lineTo(padding + (startIndex + data.length - 1) * stepX, padding + chartHeight);
        this.ctx.lineTo(padding + startIndex * stepX, padding + chartHeight);
        this.ctx.closePath();
        
        this.ctx.fillStyle = color + '20';
        this.ctx.fill();
    }
    
    clear() {
        this.data.rx = [];
        this.data.tx = [];
        this.maxValue = 10;
        this.draw();
    }
}
