/**
 * DDPico Bridge Dashboard - Main Application
 * Optimized for high-performance logging
 */

// Global state
const state = {
    paused: false,
    autoScroll: true,
    connected: false,
    startTime: Date.now(),
    lastPacketCount: { rx: 0, tx: 0 },
    lastUpdateTime: Date.now(),
    settings: {
        theme: 'dark',
        updateInterval: 100,
        maxConsoleLines: 500,
        soundEnabled: false,
        showTimestamps: true,
        tweeningEnabled: false,
        tweeningSteps: 4,
        targetFps: 60
    },
    // Performance optimization
    logBuffer: [],
    logFlushTimer: null,
    scrollDebounceTimer: null,
    lastScrollTime: 0
};

// Initialize chart
let packetChart;

// DOM Elements
const elements = {
    console: document.getElementById('console'),
    statusBadge: document.getElementById('statusBadge'),
    statusText: document.getElementById('statusText'),
    packetsRx: document.getElementById('packetsRx'),
    packetsTx: document.getElementById('packetsTx'),
    bytesRx: document.getElementById('bytesRx'),
    bytesTx: document.getElementById('bytesTx'),
    throughput: document.getElementById('throughput'),
    uptime: document.getElementById('uptime'),
    serialPort: document.getElementById('serialPort'),
    pauseBtn: document.getElementById('pauseBtn'),
    scrollBtn: document.getElementById('scrollBtn'),
    settingsModal: document.getElementById('settingsModal'),
    settingsBtn: document.getElementById('settingsBtn')
};

// Initialize on page load
document.addEventListener('DOMContentLoaded', () => {
    initializeApp();
    loadSettings();
    connectEventSource();
    startUptimeCounter();
});

function initializeApp() {
    // Initialize chart
    packetChart = new SimpleChart('packetChart', {
        maxDataPoints: 60,
        colors: {
            rx: '#3b82f6',
            tx: '#10b981'
        }
    });
    
    // Setup event listeners
    elements.settingsBtn.addEventListener('click', openSettings);
    
    addLog('[SYSTEM] DDPico Bridge Dashboard initialized');
    addLog('[CONFIG] Configure DDP software to send to 127.0.0.1:4048');
}

function connectEventSource() {
    let reconnectAttempts = 0;
    const maxReconnectAttempts = 10;
    const reconnectDelay = 2000;
    
    function connect() {
        const eventSource = new EventSource('/events');
        
        eventSource.onmessage = function(e) {
            try {
                const data = JSON.parse(e.data);
                handleServerEvent(data);
                reconnectAttempts = 0; // Reset on successful message
            } catch (err) {
                console.error('Error parsing event data:', err);
            }
        };
        
        eventSource.onerror = function(err) {
            console.error('EventSource error:', err);
            eventSource.close();
            
            if (reconnectAttempts < maxReconnectAttempts) {
                reconnectAttempts++;
                addLog(`[WARN] Connection lost. Reconnecting (${reconnectAttempts}/${maxReconnectAttempts})...`, 'warning');
                updateStatus(false);
                setTimeout(connect, reconnectDelay);
            } else {
                addLog('[ERROR] Connection to server lost. Max reconnect attempts reached.', 'error');
                updateStatus(false);
            }
        };
        
        eventSource.onopen = function() {
            addLog('[SYSTEM] Connected to server', 'success');
            reconnectAttempts = 0;
        };
        
        return eventSource;
    }
    
    return connect();
}

function handleServerEvent(data) {
    switch (data.type) {
        case 'log':
            addLog(data.message);
            break;
        case 'status':
            updateStatus(data.connected);
            if (data.serial_port) {
                elements.serialPort.textContent = data.serial_port;
            }
            break;
        case 'stats':
            updateStats(data);
            break;
    }
}

function updateStatus(connected) {
    state.connected = connected;
    elements.statusBadge.className = 'status-badge ' + (connected ? 'connected' : 'disconnected');
    elements.statusText.textContent = connected ? 'Connected' : 'Disconnected';
}

function updateStats(stats) {
    // Update packet counts
    if (stats.packets_rx !== undefined) {
        elements.packetsRx.textContent = stats.packets_rx.toLocaleString();
    }
    if (stats.packets_tx !== undefined) {
        elements.packetsTx.textContent = stats.packets_tx.toLocaleString();
    }
    
    // Update byte counts
    if (stats.bytes_rx !== undefined) {
        elements.bytesRx.innerHTML = (stats.bytes_rx / 1024).toFixed(1) + '<span class="stat-unit">KB</span>';
    }
    if (stats.bytes_tx !== undefined) {
        elements.bytesTx.innerHTML = (stats.bytes_tx / 1024).toFixed(1) + '<span class="stat-unit">KB</span>';
    }
    
    // Calculate and update throughput
    const now = Date.now();
    const timeDelta = (now - state.lastUpdateTime) / 1000;
    
    if (timeDelta > 0) {
        const rxDelta = (stats.packets_rx || 0) - state.lastPacketCount.rx;
        const txDelta = (stats.packets_tx || 0) - state.lastPacketCount.tx;
        
        const rxRate = rxDelta / timeDelta;
        const txRate = txDelta / timeDelta;
        
        // Update chart
        packetChart.addData(rxRate, txRate);
        
        // Update throughput display (using tx rate as primary metric)
        const throughputKBps = ((stats.bytes_tx || 0) - (state.lastPacketCount.txBytes || 0)) / timeDelta / 1024;
        elements.throughput.innerHTML = Math.max(0, throughputKBps).toFixed(2) + '<span class="stat-unit">KB/s</span>';
        
        state.lastPacketCount = {
            rx: stats.packets_rx || 0,
            tx: stats.packets_tx || 0,
            txBytes: stats.bytes_tx || 0
        };
        state.lastUpdateTime = now;
    }
}

function addLog(msg, type = 'info') {
    if (state.paused) return;

    try {
        const timestamp = new Date();
    const time = timestamp.toLocaleTimeString('en-US', {hour12: false}) + '.' +
                 timestamp.getMilliseconds().toString().padStart(3, '0');
    
    let className = 'log-info';
    if (msg.includes('[ERROR]') || type === 'error') {
        className = 'log-error';
    } else if (msg.includes('[SERIAL]') || msg.includes('[UDP]') || msg.includes('[BRIDGE]') || type === 'success') {
        className = 'log-success';
    } else if (msg.includes('ACK:') || msg.includes('[DDPico]')) {
        // Highlight acknowledgment messages from Pico
        className = 'log-success';
    } else if (msg.includes('[DDP]') || msg.includes('[STATS]')) {
        className = 'log-info';
    } else if (msg.includes('[WARN]') || type === 'warning') {
        className = 'log-warning';
    }
    
    // Check filters
    const filterError = document.getElementById('filterError').checked;
    const filterInfo = document.getElementById('filterInfo').checked;
    const filterDebug = document.getElementById('filterDebug').checked;
    
    if (className === 'log-error' && !filterError) return;
    if ((className === 'log-info' || className === 'log-success') && !filterInfo) return;
    if (className === 'log-debug' && !filterDebug) return;
    
    // Buffer the log entry instead of immediate DOM manipulation
    const logEntry = {
        time: time,
        className: className,
        msg: msg
    };

    state.logBuffer.push(logEntry);

    // Force flush if buffer is getting too large (emergency measure)
    if (state.logBuffer.length > 100) {
        if (state.logFlushTimer) {
            cancelAnimationFrame(state.logFlushTimer);
        }
        flushLogs();
    } else if (!state.logFlushTimer) {
        // Schedule flush if not already scheduled
        state.logFlushTimer = requestAnimationFrame(flushLogs);
    }
    } catch (error) {
        console.error('Error in addLog:', error);
        // Fallback: direct console logging
        console.log(msg);
    }
}

function flushLogs() {
    try {
        if (state.logBuffer.length === 0) {
            state.logFlushTimer = null;
            return;
        }

        // Process all buffered logs
        const fragment = document.createDocumentFragment();
        let shouldScroll = false;

        for (const entry of state.logBuffer) {
            const logLine = document.createElement('div');
            if (state.settings.showTimestamps) {
                logLine.innerHTML = `<span class="log-time">[${entry.time}]</span> <span class="${entry.className}">${escapeHtml(entry.msg)}</span>`;
            } else {
                logLine.innerHTML = `<span class="${entry.className}">${escapeHtml(entry.msg)}</span>`;
            }
            fragment.appendChild(logLine);
            shouldScroll = true;
        }

        // Clear buffer
        state.logBuffer.length = 0;
        state.logFlushTimer = null;

        // Batch DOM update
        elements.console.appendChild(fragment);

        // Auto-scroll if enabled and we added lines
        if (state.autoScroll && shouldScroll) {
            // Debounce scroll to avoid excessive updates
            if (state.scrollDebounceTimer) {
                clearTimeout(state.scrollDebounceTimer);
            }
            state.scrollDebounceTimer = setTimeout(() => {
                elements.console.scrollTop = elements.console.scrollHeight;
                state.scrollDebounceTimer = null;
            }, 16); // ~60fps
        }

        // Limit console lines efficiently
        const lines = elements.console.children;
        if (lines.length > state.settings.maxConsoleLines) {
            const removeCount = lines.length - state.settings.maxConsoleLines;
            // Remove multiple elements at once for better performance
            for (let i = 0; i < removeCount; i++) {
                elements.console.removeChild(lines[0]);
            }
        }
    } catch (error) {
        console.error('Error in flushLogs:', error);
        // Clear buffer to prevent infinite retries
        state.logBuffer.length = 0;
        state.logFlushTimer = null;
    }
}

function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

function clearLog() {
    elements.console.innerHTML = '';
    state.logBuffer.length = 0; // Clear any pending logs
    addLog('[SYSTEM] Console cleared');
}

function togglePause() {
    state.paused = !state.paused;
    elements.pauseBtn.textContent = state.paused ? 'Resume' : 'Pause';
    if (!state.paused) {
        addLog('[SYSTEM] Console resumed');
    }
}

function toggleAutoScroll() {
    state.autoScroll = !state.autoScroll;
    elements.scrollBtn.textContent = 'Auto-scroll: ' + (state.autoScroll ? 'ON' : 'OFF');
}

function exportLogs() {
    const logs = elements.console.innerText;
    const blob = new Blob([logs], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `ddpico-logs-${new Date().toISOString().replace(/[:.]/g, '-')}.txt`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
    addLog('[SYSTEM] Logs exported');
}

function startUptimeCounter() {
    setInterval(() => {
        const uptime = Date.now() - state.startTime;
        const hours = Math.floor(uptime / 3600000);
        const minutes = Math.floor((uptime % 3600000) / 60000);
        const seconds = Math.floor((uptime % 60000) / 1000);
        
        elements.uptime.textContent = 
            `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;
    }, 1000);
}

// Settings Modal Functions
function openSettings() {
    elements.settingsModal.classList.add('active');

    // Load current settings
    document.getElementById('themeSelect').value = state.settings.theme;
    document.getElementById('updateInterval').value = state.settings.updateInterval;
    document.getElementById('maxConsoleLines').value = state.settings.maxConsoleLines;
    document.getElementById('soundEnabled').checked = state.settings.soundEnabled;
    document.getElementById('showTimestamps').checked = state.settings.showTimestamps;

    // Fetch tweening settings from server
    fetch('/settings')
        .then(response => response.json())
        .then(data => {
            document.getElementById('tweeningEnabled').checked = data.tweening_enabled;
            document.getElementById('tweeningSteps').value = data.tweening_steps;
            document.getElementById('targetFps').value = data.target_fps;
        })
        .catch(error => {
            console.error('Error fetching tweening settings:', error);
            // Use local defaults
            document.getElementById('tweeningEnabled').checked = state.settings.tweeningEnabled;
            document.getElementById('tweeningSteps').value = state.settings.tweeningSteps;
            document.getElementById('targetFps').value = state.settings.targetFps;
        });
}

function closeSettings() {
    elements.settingsModal.classList.remove('active');
}

function saveSettings() {
    state.settings.theme = document.getElementById('themeSelect').value;
    state.settings.updateInterval = parseInt(document.getElementById('updateInterval').value);
    state.settings.maxConsoleLines = parseInt(document.getElementById('maxConsoleLines').value);
    state.settings.soundEnabled = document.getElementById('soundEnabled').checked;
    state.settings.showTimestamps = document.getElementById('showTimestamps').checked;

    // Tweening settings
    const tweeningSettings = {
        tweening_enabled: document.getElementById('tweeningEnabled').checked,
        tweening_steps: parseInt(document.getElementById('tweeningSteps').value),
        target_fps: parseInt(document.getElementById('targetFps').value)
    };

    // Save tweening settings to server
    fetch('/settings', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(tweeningSettings)
    })
    .then(response => {
        if (response.ok) {
            addLog('[SYSTEM] Tweening settings updated');
        } else {
            throw new Error('Failed to save tweening settings');
        }
    })
    .catch(error => {
        console.error('Error saving tweening settings:', error);
        addLog('[ERROR] Failed to save tweening settings', 'error');
    });

    // Save local settings to localStorage
    localStorage.setItem('ddpico-settings', JSON.stringify(state.settings));

    addLog('[SYSTEM] Settings saved');
    closeSettings();

    // Apply theme if changed
    applyTheme();
}

function loadSettings() {
    const saved = localStorage.getItem('ddpico-settings');
    if (saved) {
        try {
            state.settings = { ...state.settings, ...JSON.parse(saved) };
            applyTheme();
        } catch (e) {
            console.error('Error loading settings:', e);
        }
    }
}

function applyTheme() {
    const theme = state.settings.theme;

    // Remove existing theme classes
    document.body.classList.remove('light-theme', 'dark-theme');

    if (theme === 'light') {
        document.body.classList.add('light-theme');
    } else if (theme === 'dark') {
        document.body.classList.add('dark-theme');
    } else if (theme === 'auto') {
        // Auto theme based on system preference
        if (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) {
            document.body.classList.add('dark-theme');
        } else {
            document.body.classList.add('light-theme');
        }

        // Listen for system theme changes
        const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)');
        const handleChange = (e) => {
            document.body.classList.remove('light-theme', 'dark-theme');
            document.body.classList.add(e.matches ? 'dark-theme' : 'light-theme');
        };

        // Remove existing listener if any
        if (mediaQuery.removeEventListener) {
            mediaQuery.removeEventListener('change', handleChange);
        }

        // Add new listener
        if (mediaQuery.addEventListener) {
            mediaQuery.addEventListener('change', handleChange);
        }
    }
}

// Close modal when clicking outside
elements.settingsModal.addEventListener('click', (e) => {
    if (e.target === elements.settingsModal) {
        closeSettings();
    }
});

// Keyboard shortcuts
document.addEventListener('keydown', (e) => {
    // Ctrl/Cmd + K to clear console
    if ((e.ctrlKey || e.metaKey) && e.key === 'k') {
        e.preventDefault();
        clearLog();
    }
    
    // Ctrl/Cmd + P to pause
    if ((e.ctrlKey || e.metaKey) && e.key === 'p') {
        e.preventDefault();
        togglePause();
    }
    
    // Escape to close modal
    if (e.key === 'Escape') {
        closeSettings();
    }
});
