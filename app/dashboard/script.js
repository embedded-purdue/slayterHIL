// ---------------------------------------------------------------------------
// RC Command Conversion (mirrors rc_conversion.py logic in the browser)
// ---------------------------------------------------------------------------

const CONVERSION_FACTOR = 0.5;

function interleaveCommands(axisCommands) {
    const active = axisCommands.filter(([, steps]) => steps > 0);
    if (!active.length) return [];

    const maxSteps = Math.max(...active.map(([, s]) => s));
    const errors = new Array(active.length).fill(0);
    const result = [];

    for (let i = 0; i < maxSteps; i++) {
        active.forEach(([cmd, steps], idx) => {
            errors[idx] += steps;
            if (errors[idx] >= maxSteps) {
                result.push(cmd);
                errors[idx] -= maxSteps;
            }
        });
    }
    return result;
}

function deltasToCommands(dx, dy, dz) {
    const xSteps = Math.round(Math.abs(dx) / CONVERSION_FACTOR);
    const ySteps = Math.round(Math.abs(dy) / CONVERSION_FACTOR);
    const zSteps = Math.round(Math.abs(dz) / CONVERSION_FACTOR);

    const xCmd = dx >= 0 ? 'R' : 'L';
    const yCmd = dy >= 0 ? 'F' : 'B';
    const zCmd = dz >= 0 ? 'U' : 'D';

    return interleaveCommands([[xCmd, xSteps], [yCmd, ySteps], [zCmd, zSteps]]);
}

function waypointsToRcCommands(waypoints) {
    const commands = [];
    for (let i = 1; i < waypoints.length; i++) {
        const prev = waypoints[i - 1];
        const curr = waypoints[i];
        commands.push(...deltasToCommands(
            curr.x - prev.x,
            curr.y - prev.y,
            curr.z - prev.z
        ));
    }
    return commands;
}

function parseCsvWaypoints(csvText) {
    const lines = csvText.trim().split('\n');
    const headers = lines[0].split(',').map(h => h.trim().toLowerCase());
    const xIdx = headers.findIndex(h => ['x', 'x_pos'].includes(h));
    const yIdx = headers.findIndex(h => ['y', 'y_pos'].includes(h));
    const zIdx = headers.findIndex(h => ['z', 'z_pos'].includes(h));

    if (xIdx < 0 || yIdx < 0 || zIdx < 0) {
        throw new Error('CSV must have x, y, z columns (or x_pos, y_pos, z_pos).');
    }

    return lines.slice(1).filter(l => l.trim()).map(line => {
        const cols = line.split(',').map(c => c.trim());
        return { x: parseFloat(cols[xIdx]), y: parseFloat(cols[yIdx]), z: parseFloat(cols[zIdx]) };
    });
}

// ---------------------------------------------------------------------------
// Dashboard class
// ---------------------------------------------------------------------------

class TestAutomationDashboard {
    constructor() {
        this.runButton = document.querySelector('.run-btn');
        this.logTableBody = document.getElementById('logTableBody');
        this.testItems = document.querySelectorAll('.test-item');
        this.waypoints = [];

        this.initializeEventListeners();
        this.setupWebSocket();
        this.initCustomInput();
    }

    initializeEventListeners() {
        if (this.runButton) {
            this.runButton.addEventListener('click', () => this.runTest());
        }

        // Add click handlers to test items
        this.testItems.forEach(item => {
            item.addEventListener('click', () => {
                this.selectTest(item);
            });
        });
    }

    // -----------------------------------------------------------------------
    // Custom input panel
    // -----------------------------------------------------------------------

    initCustomInput() {
        // Tab switching
        document.querySelectorAll('.input-tab-btn').forEach(btn => {
            btn.addEventListener('click', () => {
                document.querySelectorAll('.input-tab-btn').forEach(b => b.classList.remove('active'));
                document.querySelectorAll('.input-tab-content').forEach(c => c.classList.remove('active'));
                btn.classList.add('active');
                document.getElementById('tab-' + btn.dataset.tab).classList.add('active');
            });
        });

        // Waypoint management
        document.getElementById('addWaypointBtn').addEventListener('click', () => this.addWaypoint());
        document.getElementById('clearWaypointsBtn').addEventListener('click', () => this.clearWaypoints());
        document.getElementById('convertWaypointsBtn').addEventListener('click', () => this.convertWaypointsToRc());

        // Raw data
        document.getElementById('convertRawBtn').addEventListener('click', () => this.convertRawToRc());

        // Send to server
        document.getElementById('sendToServerBtn').addEventListener('click', () => this.sendRcToServer());
    }

    addWaypoint() {
        const x = parseFloat(document.getElementById('wp-x').value) || 0;
        const y = parseFloat(document.getElementById('wp-y').value) || 0;
        const z = parseFloat(document.getElementById('wp-z').value) || 0;

        this.waypoints.push({ x, y, z });
        this.renderWaypointsTable();

        // Clear inputs
        ['wp-x', 'wp-y', 'wp-z'].forEach(id => document.getElementById(id).value = '');
    }

    clearWaypoints() {
        this.waypoints = [];
        this.renderWaypointsTable();
        document.getElementById('rcOutput').style.display = 'none';
    }

    renderWaypointsTable() {
        const tbody = document.getElementById('waypointsBody');
        tbody.innerHTML = '';
        this.waypoints.forEach((wp, idx) => {
            const tr = document.createElement('tr');
            tr.innerHTML = `
                <td>${idx + 1}</td>
                <td>${wp.x}</td>
                <td>${wp.y}</td>
                <td>${wp.z}</td>
                <td><button class="remove-wp-btn" data-idx="${idx}">✕</button></td>
            `;
            tbody.appendChild(tr);
        });

        tbody.querySelectorAll('.remove-wp-btn').forEach(btn => {
            btn.addEventListener('click', () => {
                this.waypoints.splice(parseInt(btn.dataset.idx), 1);
                this.renderWaypointsTable();
            });
        });
    }

    convertWaypointsToRc() {
        if (this.waypoints.length < 2) {
            alert('Add at least 2 waypoints to generate commands.');
            return;
        }
        const commands = waypointsToRcCommands(this.waypoints);
        this.showRcOutput(commands);
    }

    convertRawToRc() {
        const text = document.getElementById('rawDataInput').value.trim();
        const format = document.getElementById('rawFormatSelect').value;
        if (!text) { alert('Paste data first.'); return; }

        try {
            let waypoints;
            if (format === 'json') {
                const data = JSON.parse(text);
                waypoints = data.map(r => ({
                    x: r.X_pos ?? r.x_pos ?? r.x ?? 0,
                    y: r.Y_pos ?? r.y_pos ?? r.y ?? 0,
                    z: r.Z_pos ?? r.z_pos ?? r.z ?? 0,
                }));
            } else {
                waypoints = parseCsvWaypoints(text);
            }
            const commands = waypointsToRcCommands(waypoints);
            this.showRcOutput(commands);
        } catch (e) {
            alert('Parse error: ' + e.message);
        }
    }

    showRcOutput(commands) {
        const output = commands.join('');
        document.getElementById('rcResult').textContent = output || '(no movement)';
        document.getElementById('rcMeta').textContent = `Total commands: ${commands.length}`;
        document.getElementById('rcOutput').style.display = 'block';
        this._lastRcCommands = commands;
    }

    sendRcToServer() {
        if (!this._lastRcCommands) return;
        const payload = { command: 'rcCommands', commands: this._lastRcCommands };
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify(payload));
            const ts = new Date().toLocaleTimeString('en-US', { hour12: false });
            this.addLogEntry(ts, 'info', 'user-input', `Sent ${this._lastRcCommands.length} RC commands to server`);
        } else {
            alert('Not connected to test server.');
        }
    }

    setupWebSocket() {
        // Replace with your actual WebSocket server URL
        try {
            this.ws = new WebSocket('ws://localhost:8080');

            this.ws.onopen = () => {
                this.addLogEntry('01:15:00', 'info', 'setup', 'Connected to test server');
            };

            this.ws.onmessage = (event) => {
                const message = JSON.parse(event.data);
                this.handleServerMessage(message);
            };

            this.ws.onerror = (error) => {
                console.log('WebSocket error:', error);
            };

            this.ws.onclose = () => {
                console.log('Disconnected from test server');
                // Attempt to reconnect after 5 seconds
                setTimeout(() => this.setupWebSocket(), 5000);
            };
        } catch (error) {
            console.log('WebSocket connection failed, running in standalone mode');
        }
    }

    selectTest(item) {
        // Remove active class from all items
        this.testItems.forEach(i => i.classList.remove('selected'));
        item.classList.add('selected');
    }

    runTest() {
        const timestamp = new Date().toLocaleTimeString('en-US', { hour12: false });
        
        this.addLogEntry(timestamp, 'info', 'setup', 'Test execution started');
        
        // Simulate test execution
        setTimeout(() => {
            this.addLogEntry(timestamp, 'info', 'execute', 'Running test cases...');
        }, 1000);
        
        setTimeout(() => {
            this.addLogEntry(timestamp, 'warn', 'execute', 'Network latency detected');
        }, 2000);
        
        setTimeout(() => {
            this.addLogEntry(timestamp, 'info', 'teardown', 'Test completed successfully');
        }, 3000);
        
        // Send to server if connected
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify({
                command: 'runTest',
                testId: 'search-test'
            }));
        }
    }

    handleServerMessage(message) {
        switch (message.type) {
            case 'log':
                this.addLogEntry(
                    message.timestamp || new Date().toLocaleTimeString('en-US', { hour12: false }),
                    message.level,
                    message.step,
                    message.message
                );
                break;
            case 'status':
                console.log('Status:', message.status);
                break;
        }
    }

    addLogEntry(time, level, step, message) {
        const row = document.createElement('tr');
        row.innerHTML = `
            <td>${time}</td>
            <td><span class="log-level ${level}">${level.toUpperCase()}</span></td>
            <td>${step}</td>
            <td>${message}</td>
        `;
        this.logTableBody.appendChild(row);
        
        // Auto-scroll to bottom
        const logsSection = document.querySelector('.logs-section');
        if (logsSection) {
            logsSection.scrollTop = logsSection.scrollHeight;
        }
    }
}

// Initialize the dashboard when the page loads
document.addEventListener('DOMContentLoaded', () => {
    window.dashboard = new TestAutomationDashboard();
});
