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
// 3D Visualizer (Three.js)
// ---------------------------------------------------------------------------

class Visualizer3D {
    constructor(containerId) {
        this.container = document.getElementById(containerId);
        this.initialized = false;
        this._scale = 150;      // half-height in world units (controls zoom level)
        this._labelObjects = []; // tracked so we can remove/rebuild on zoom
    }

    _init() {
        if (this.initialized) return;
        this.initialized = true;

        this.container.innerHTML = '';

        const w = this.container.clientWidth;
        const h = this.container.clientHeight;
        const aspect = w / h;

        this.scene = new THREE.Scene();
        this.scene.background = new THREE.Color(0x0a0a0a);

        // Orthographic camera: zoom changes frustum size, not camera distance
        this.camera = new THREE.OrthographicCamera(
            -this._scale * aspect,  // left
             this._scale * aspect,  // right
             this._scale,           // top
            -this._scale,           // bottom
            0.1, 100000
        );
        // Position camera at an angle — direction matters for ortho, distance does not
        this.camera.position.set(500, 375, 500);
        this.camera.lookAt(0, 0, 0);

        // WebGL renderer
        this.renderer = new THREE.WebGLRenderer({ antialias: true });
        this.renderer.setPixelRatio(window.devicePixelRatio);
        this.renderer.setSize(w, h);
        this.container.appendChild(this.renderer.domElement);

        // CSS2D renderer — overlays HTML labels in 3D space
        this.labelRenderer = new THREE.CSS2DRenderer();
        this.labelRenderer.setSize(w, h);
        this.labelRenderer.domElement.style.position = 'absolute';
        this.labelRenderer.domElement.style.top = '0';
        this.labelRenderer.domElement.style.left = '0';
        this.labelRenderer.domElement.style.pointerEvents = 'none';
        this.container.appendChild(this.labelRenderer.domElement);

        // OrbitControls: disable built-in zoom — we handle it ourselves
        this.controls = new THREE.OrbitControls(this.camera, this.renderer.domElement);
        this.controls.enableDamping = true;
        this.controls.dampingFactor = 0.08;
        this.controls.enableZoom = false;

        // Scroll → adjust frustum size (scale) and rebuild tick labels
        this.renderer.domElement.addEventListener('wheel', (e) => {
            e.preventDefault();
            const factor = e.deltaY > 0 ? 1.15 : 1 / 1.15;
            this._scale = Math.max(1, Math.min(50000, this._scale * factor));
            this._updateFrustum();
            this._rebuildAxisLabels();
        }, { passive: false });

        this.scene.add(new THREE.AmbientLight(0xffffff, 0.7));
        const dir = new THREE.DirectionalLight(0xffffff, 0.6);
        dir.position.set(1, 2, 1);
        this.scene.add(dir);

        // Long axis lines that always extend beyond the viewport
        this._buildAxes(10000);
        this._rebuildAxisLabels();

        this.pathGroup = new THREE.Group();
        this.scene.add(this.pathGroup);

        window.addEventListener('resize', () => this._onResize());
        this._animate();
    }

    // Recompute and apply the orthographic frustum from current _scale
    _updateFrustum() {
        const aspect = this.container.clientWidth / this.container.clientHeight;
        this.camera.left   = -this._scale * aspect;
        this.camera.right  =  this._scale * aspect;
        this.camera.top    =  this._scale;
        this.camera.bottom = -this._scale;
        this.camera.updateProjectionMatrix();
    }

    // Build long axis lines (direction sets color, length >> any expected scene)
    _buildAxes(len) {
        const addLine = (ax, color) => {
            const pts = [ax.clone().multiplyScalar(-len), ax.clone().multiplyScalar(len)];
            const geo = new THREE.BufferGeometry().setFromPoints(pts);
            this.scene.add(new THREE.Line(geo, new THREE.LineBasicMaterial({ color })));
        };
        addLine(new THREE.Vector3(1, 0,  0), 0xff5555); // X
        addLine(new THREE.Vector3(0, 1,  0), 0x55ff55); // Z (up)
        addLine(new THREE.Vector3(0, 0, -1), 0x5599ff); // Y (fwd)
    }

    _makeLabel(text, color) {
        const div = document.createElement('div');
        div.textContent = text;
        div.style.cssText = `
            color: ${color};
            font-size: 11px;
            font-family: 'Segoe UI', monospace, sans-serif;
            background: rgba(10,10,10,0.75);
            padding: 1px 5px;
            border-radius: 3px;
            pointer-events: none;
            white-space: nowrap;
            user-select: none;
        `;
        return new THREE.CSS2DObject(div);
    }

    // Compute a clean tick spacing (1/2/5 × 10^n) for the current visible range
    _tickSpacing() {
        const raw = this._scale * 0.35; // aim for ~3 ticks in the positive half
        const mag = Math.pow(10, Math.floor(Math.log10(Math.max(raw, 1e-9))));
        const n   = raw / mag;
        const step = n < 1.5 ? 1 : n < 3.5 ? 2 : n < 7.5 ? 5 : 10;
        return step * mag;
    }

    // Format a tick value cleanly (no unnecessary decimals)
    _fmt(v) {
        if (v === 0) return '0';
        // tick values are always 1/2/5 × 10^n so toPrecision(2) is exact
        const s = parseFloat(v.toPrecision(2));
        return Number.isInteger(s) ? String(s) : s.toString();
    }

    // Remove old dynamic labels and regenerate based on current _scale / tick spacing
    _rebuildAxisLabels() {
        this._labelObjects.forEach(o => this.scene.remove(o));
        this._labelObjects = [];

        const tick    = this._tickSpacing();
        const maxTick = 18; // max ticks per axis direction
        const maxVal  = tick * maxTick;

        const add = (text, color, x, y, z) => {
            const lbl = this._makeLabel(text, color);
            lbl.position.set(x, y, z);
            this.scene.add(lbl);
            this._labelObjects.push(lbl);
        };

        // Axis name labels placed 6 ticks out
        const nameOff = tick * 6;
        add('X', '#ff9999', nameOff, 0, 0);
        add('Z', '#99ff99', 0, nameOff, 0);
        add('Y', '#99bbff', 0, 0, -nameOff);

        // Tick numbers — skip 0, generate ±values along each axis
        for (let v = tick; v <= maxVal; v += tick) {
            const f = this._fmt(v);
            add( f, '#ff7777',  v,  0,  0);  // +X
            add('-' + f, '#ff7777', -v,  0,  0);  // -X
            add( f, '#77ff77',  0,  v,  0);  // +Z (up)
            add( f, '#77aaff',  0,  0, -v);  // +Y (forward)
            add('-' + f, '#77aaff',  0,  0,  v);  // -Y (back)
        }
    }

    _toThree(wp) {
        return new THREE.Vector3(wp.x, wp.z, -wp.y);
    }

    update(waypoints) {
        if (!waypoints.length) {
            if (this.initialized) this.pathGroup.clear();
            return;
        }

        this._init();
        this.pathGroup.clear();

        const pts = waypoints.map(wp => this._toThree(wp));

        const lineGeo = new THREE.BufferGeometry().setFromPoints(pts);
        this.pathGroup.add(new THREE.Line(lineGeo, new THREE.LineBasicMaterial({ color: 0x14b8a6 })));

        const sphereGeo = new THREE.SphereGeometry(2, 14, 14);
        pts.forEach((pt, i) => {
            const color = i === 0              ? 0x14b8a6
                        : i === pts.length - 1 ? 0xd97706
                        :                        0x4a7bc0;
            const mesh = new THREE.Mesh(sphereGeo, new THREE.MeshPhongMaterial({ color }));
            mesh.position.copy(pt);
            this.pathGroup.add(mesh);
        });

        // Auto-fit: adjust frustum scale to frame all waypoints
        const box    = new THREE.Box3().setFromPoints(pts);
        const center = box.getCenter(new THREE.Vector3());
        const size   = box.getSize(new THREE.Vector3());
        const span   = Math.max(size.x, size.y, size.z, 20);

        this._scale = span * 1.6;
        this._updateFrustum();
        this._rebuildAxisLabels();

        this.controls.target.copy(center);
        this.controls.update();
    }

    _animate() {
        requestAnimationFrame(() => this._animate());
        this.controls.update();
        this.renderer.render(this.scene, this.camera);
        this.labelRenderer.render(this.scene, this.camera);
    }

    _onResize() {
        if (!this.initialized) return;
        const w = this.container.clientWidth;
        const h = this.container.clientHeight;
        this._updateFrustum();
        this.renderer.setSize(w, h);
        this.labelRenderer.setSize(w, h);
    }
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

        this.viz = new Visualizer3D('viz3d');

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

        this.viz.update(this.waypoints);
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
            this.viz.update(waypoints);
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
