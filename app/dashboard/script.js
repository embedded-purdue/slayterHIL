// ---------------------------------------------------------------------------
// Generic conversion (mirrors tests/conversionsTests/generic_conversion.py)
// ---------------------------------------------------------------------------

const GENERIC_CONVERSION_FACTOR = 7.5;

function gcd(a, b) {
    a = Math.abs(Math.round(a)) | 0;
    b = Math.abs(Math.round(b)) | 0;
    if (a === 0 && b === 0) return 1;
    while (b !== 0) {
        const t = b;
        b = a % b;
        a = t;
    }
    return a || 1;
}

/**
 * @param {Array<{X_pos:number,Y_pos:number,Z_pos:number}>} data
 * @returns {string[]}
 */
function genericConvertToCommandList(data) {
    const commands = [];

    for (let i = 1; i < data.length; i++) {
        const prev = data[i - 1];
        const curr = data[i];
        const dx = curr.X_pos - prev.X_pos;
        const dy = curr.Y_pos - prev.Y_pos;
        const dz = curr.Z_pos - prev.Z_pos;

        let changes = 0;
        if (dx !== 0) changes += 1;
        if (dy !== 0) changes += 1;
        if (dz !== 0) changes += 1;

        if (changes === 0) {
            commands.push('I');
        }

        if (changes > 0 && changes < 2) {
            if (dz !== 0) {
                const steps = Math.trunc(Math.abs(dz) / GENERIC_CONVERSION_FACTOR);
                for (let k = 0; k < steps; k++) commands.push(dz > 0 ? 'U' : 'D');
            }
            if (dx !== 0) {
                const steps = Math.trunc(Math.abs(dx) / GENERIC_CONVERSION_FACTOR);
                for (let k = 0; k < steps; k++) commands.push(dx > 0 ? 'R' : 'L');
            }
            if (dy !== 0) {
                const steps = Math.trunc(Math.abs(dy) / GENERIC_CONVERSION_FACTOR);
                for (let k = 0; k < steps; k++) commands.push(dy > 0 ? 'F' : 'B');
            }
        }

        if (changes === 2) {
            const xsteps = Math.round(Math.abs(dx) / GENERIC_CONVERSION_FACTOR);
            const ysteps = Math.round(Math.abs(dy) / GENERIC_CONVERSION_FACTOR);
            const zsteps = Math.round(Math.abs(dz) / GENERIC_CONVERSION_FACTOR);

            if (dz !== 0 && dx !== 0) {
                const factor = gcd(zsteps, xsteps);
                const r1 = Math.trunc(zsteps / factor);
                const r2 = Math.trunc(xsteps / factor);
                let total1steps = 0;
                let total2steps = 0;
                while (total1steps < zsteps || total2steps < xsteps) {
                    for (let j = 0; j < r1; j++) {
                        commands.push(dz > 0 ? 'U' : 'D');
                        total1steps += 1;
                    }
                    for (let j = 0; j < r2; j++) {
                        commands.push(dx > 0 ? 'R' : 'L');
                        total2steps += 1;
                    }
                }
            }
            if (dz !== 0 && dy !== 0) {
                const factor = gcd(zsteps, ysteps);
                const r1 = Math.trunc(zsteps / factor);
                const r2 = Math.trunc(ysteps / factor);
                let total1steps = 0;
                let total2steps = 0;
                while (total1steps < zsteps || total2steps < ysteps) {
                    for (let j = 0; j < r1; j++) {
                        commands.push(dz > 0 ? 'U' : 'D');
                        total1steps += 1;
                    }
                    for (let j = 0; j < r2; j++) {
                        commands.push(dy > 0 ? 'F' : 'B');
                        total2steps += 1;
                    }
                }
            }
            if (dx !== 0 && dy !== 0) {
                const factor = gcd(xsteps, ysteps);
                const r1 = Math.trunc(xsteps / factor);
                const r2 = Math.trunc(ysteps / factor);
                let total1steps = 0;
                let total2steps = 0;
                while (total1steps < xsteps || total2steps < ysteps) {
                    for (let j = 0; j < r1; j++) {
                        commands.push(dx > 0 ? 'R' : 'L');
                        total1steps += 1;
                    }
                    for (let j = 0; j < r2; j++) {
                        commands.push(dy > 0 ? 'F' : 'B');
                        total2steps += 1;
                    }
                }
            }
        }

        if (changes === 3) {
            let xsteps = Math.round(Math.abs(dx) / GENERIC_CONVERSION_FACTOR);
            let ysteps = Math.round(Math.abs(dy) / GENERIC_CONVERSION_FACTOR);
            let zsteps = Math.round(Math.abs(dz) / GENERIC_CONVERSION_FACTOR);
            const f1 = gcd(xsteps, ysteps);
            const factor = gcd(f1, zsteps);
            const r1 = Math.trunc(xsteps / factor);
            const r2 = Math.trunc(ysteps / factor);
            const r3 = Math.trunc(zsteps / factor);
            let totalxsteps = 0;
            let totalysteps = 0;
            let totalzsteps = 0;
            while (totalxsteps < xsteps || totalysteps < ysteps || totalzsteps < zsteps) {
                for (let j = 0; j < r3; j++) {
                    if (dz > 0) commands.push('U');
                    else commands.push('D');
                    totalzsteps += 1;
                }
                for (let j = 0; j < r1; j++) {
                    if (dz > 0) commands.push('R');
                    else commands.push('L');
                    totalxsteps += 1;
                }
                for (let j = 0; j < r2; j++) {
                    if (dy > 0) commands.push('F');
                    else commands.push('B');
                    totalysteps += 1;
                }
            }
        }
    }

    return commands;
}

/** Display string: dots after R, L, F, B; I.; U and D unchanged. */
function formatGenericRcDisplay(commandList) {
    return commandList
        .map((c) => {
            if (c === 'R' || c === 'L' || c === 'F' || c === 'B') return `${c}.`;
            if (c === 'I') return 'I.';
            return c;
        })
        .join('');
}

function jsonRecordsToWaypoints(data) {
    return data.map((r) => ({
        x: r.X_pos ?? r.x ?? 0,
        y: r.Y_pos ?? r.y ?? 0,
        z: r.Z_pos ?? r.z ?? 0,
    }));
}

// ---------------------------------------------------------------------------
// Custom input RC (unchanged behavior for manual waypoints and raw paste)
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

    const xCmd = dx >= 0 ? 'R.' : 'L.';
    const yCmd = dy >= 0 ? 'F.' : 'B.';
    const zCmd = dz >= 0 ? 'U' : 'D';

    return interleaveCommands([
        [zCmd, zSteps],
        [xCmd, xSteps],
        [yCmd, ySteps],
    ]);
}

function waypointsToRcCommands(waypoints) {
    const commands = [];
    for (let i = 1; i < waypoints.length; i++) {
        const prev = waypoints[i - 1];
        const curr = waypoints[i];
        commands.push(
            ...deltasToCommands(curr.x - prev.x, curr.y - prev.y, curr.z - prev.z)
        );
    }
    return commands;
}

function parseCsvWaypoints(csvText) {
    const lines = csvText.trim().split('\n');
    const headers = lines[0].split(',').map((h) => h.trim().toLowerCase());
    const xIdx = headers.findIndex((h) => ['x', 'x_pos'].includes(h));
    const yIdx = headers.findIndex((h) => ['y', 'y_pos'].includes(h));
    const zIdx = headers.findIndex((h) => ['z', 'z_pos'].includes(h));

    if (xIdx < 0 || yIdx < 0 || zIdx < 0) {
        throw new Error('CSV must have x, y, z columns (or x_pos, y_pos, z_pos).');
    }

    return lines
        .slice(1)
        .filter((l) => l.trim())
        .map((line) => {
            const cols = line.split(',').map((c) => c.trim());
            return {
                x: parseFloat(cols[xIdx]),
                y: parseFloat(cols[yIdx]),
                z: parseFloat(cols[zIdx]),
            };
        });
}

// ---------------------------------------------------------------------------
// 3D Visualizer (Three.js)
// ---------------------------------------------------------------------------

function clearObject3DGroup(group) {
    while (group.children.length) {
        const o = group.children[0];
        group.remove(o);
        if (o.geometry) o.geometry.dispose();
        if (o.material) {
            if (Array.isArray(o.material)) o.material.forEach((m) => m.dispose());
            else o.material.dispose();
        }
    }
}

class Visualizer3D {
    constructor(containerId) {
        this.container = document.getElementById(containerId);
        this.initialized = false;
        this._scale = 150;
        this._labelObjects = [];
        this.pathGroup = null;
    }

    _init() {
        if (this.initialized) return;
        this.initialized = true;

        this.container.innerHTML = '';

        this._tooltip = document.createElement('div');
        this._tooltip.style.cssText = `
            position: absolute;
            background: rgba(10,10,10,0.92);
            color: #e0e0e0;
            font-size: 11px;
            font-family: 'Segoe UI', monospace, sans-serif;
            padding: 4px 8px;
            border-radius: 4px;
            border: 1px solid rgba(255,255,255,0.15);
            pointer-events: none;
            display: none;
            white-space: nowrap;
            z-index: 10;
        `;
        this.container.appendChild(this._tooltip);

        this._raycaster = new THREE.Raycaster();
        this._mouse = new THREE.Vector2();
        this._waypointMeshes = [];

        const w = this.container.clientWidth;
        const h = this.container.clientHeight;
        const aspect = w / h;

        this.scene = new THREE.Scene();
        this.scene.background = new THREE.Color(0x0a0a0a);

        this.camera = new THREE.OrthographicCamera(
            -this._scale * aspect,
            this._scale * aspect,
            this._scale,
            -this._scale,
            0.1,
            100000
        );
        this.camera.position.set(500, 375, 500);
        this.camera.lookAt(0, 0, 0);

        this.renderer = new THREE.WebGLRenderer({ antialias: true });
        this.renderer.setPixelRatio(window.devicePixelRatio);
        this.renderer.setSize(w, h);
        this.container.appendChild(this.renderer.domElement);

        this.labelRenderer = new THREE.CSS2DRenderer();
        this.labelRenderer.setSize(w, h);
        this.labelRenderer.domElement.style.position = 'absolute';
        this.labelRenderer.domElement.style.top = '0';
        this.labelRenderer.domElement.style.left = '0';
        this.labelRenderer.domElement.style.pointerEvents = 'none';
        this.container.appendChild(this.labelRenderer.domElement);

        this.controls = new THREE.OrbitControls(this.camera, this.renderer.domElement);
        this.controls.enableDamping = true;
        this.controls.dampingFactor = 0.08;
        this.controls.enableZoom = false;

        this.renderer.domElement.addEventListener('mousemove', (e) => this._onHover(e));
        this.renderer.domElement.addEventListener('mouseleave', () => {
            this._tooltip.style.display = 'none';
            this.renderer.domElement.style.cursor = '';
        });

        this.renderer.domElement.addEventListener(
            'wheel',
            (e) => {
                e.preventDefault();
                const factor = e.deltaY > 0 ? 1.15 : 1 / 1.15;
                this._scale = Math.max(1, Math.min(50000, this._scale * factor));
                this._updateFrustum();
                this._rebuildAxisLabels();
            },
            { passive: false }
        );

        this.scene.add(new THREE.AmbientLight(0xffffff, 0.7));
        const dir = new THREE.DirectionalLight(0xffffff, 0.6);
        dir.position.set(1, 2, 1);
        this.scene.add(dir);

        this._buildAxes(10000);
        this._rebuildAxisLabels();

        this.pathGroup = new THREE.Group();
        this.scene.add(this.pathGroup);

        window.addEventListener('resize', () => this._onResize());
        this._animate();
    }

    _updateFrustum() {
        const aspect = this.container.clientWidth / this.container.clientHeight;
        this.camera.left = -this._scale * aspect;
        this.camera.right = this._scale * aspect;
        this.camera.top = this._scale;
        this.camera.bottom = -this._scale;
        this.camera.updateProjectionMatrix();
    }

    _buildAxes(len) {
        const addLine = (ax, color) => {
            const pts = [ax.clone().multiplyScalar(-len), ax.clone().multiplyScalar(len)];
            const geo = new THREE.BufferGeometry().setFromPoints(pts);
            this.scene.add(new THREE.Line(geo, new THREE.LineBasicMaterial({ color })));
        };
        addLine(new THREE.Vector3(1, 0, 0), 0xff5555);
        addLine(new THREE.Vector3(0, 1, 0), 0x55ff55);
        addLine(new THREE.Vector3(0, 0, -1), 0x5599ff);
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

    _tickSpacing() {
        const raw = this._scale * 0.35;
        const mag = Math.pow(10, Math.floor(Math.log10(Math.max(raw, 1e-9))));
        const n = raw / mag;
        const step = n < 1.5 ? 1 : n < 3.5 ? 2 : n < 7.5 ? 5 : 10;
        return step * mag;
    }

    _fmt(v) {
        if (v === 0) return '0';
        const s = parseFloat(v.toPrecision(2));
        return Number.isInteger(s) ? String(s) : s.toString();
    }

    _rebuildAxisLabels() {
        this._labelObjects.forEach((o) => this.scene.remove(o));
        this._labelObjects = [];

        const tick = this._tickSpacing();
        const maxTick = 18;
        const maxVal = tick * maxTick;

        const add = (text, color, x, y, z) => {
            const lbl = this._makeLabel(text, color);
            lbl.position.set(x, y, z);
            this.scene.add(lbl);
            this._labelObjects.push(lbl);
        };

        const nameOff = tick * 6;
        add('X', '#ff9999', nameOff, 0, 0);
        add('Z', '#99ff99', 0, nameOff, 0);
        add('Y', '#99bbff', 0, 0, -nameOff);

        for (let v = tick; v <= maxVal; v += tick) {
            const f = this._fmt(v);
            add(f, '#ff7777', v, 0, 0);
            add(`-${f}`, '#ff7777', -v, 0, 0);
            add(f, '#77ff77', 0, v, 0);
            add(f, '#77aaff', 0, 0, -v);
            add(`-${f}`, '#77aaff', 0, 0, v);
        }
    }

    _onHover(e) {
        if (!this._waypointMeshes || !this._waypointMeshes.length) return;

        const rect = this.renderer.domElement.getBoundingClientRect();
        this._mouse.x = ((e.clientX - rect.left) / rect.width) * 2 - 1;
        this._mouse.y = -((e.clientY - rect.top) / rect.height) * 2 + 1;

        this._raycaster.setFromCamera(this._mouse, this.camera);
        const hits = this._raycaster.intersectObjects(this._waypointMeshes);

        if (hits.length) {
            const { stepIndex, waypoint: wp } = hits[0].object.userData;
            const fmt = (v) => (Number.isFinite(v) ? parseFloat(v.toFixed(3)) : v);
            this._tooltip.textContent = `Step ${stepIndex + 1}  |  X: ${fmt(wp.x)}  Y: ${fmt(wp.y)}  Z: ${fmt(wp.z)}`;
            this._tooltip.style.left = `${e.offsetX + 14}px`;
            this._tooltip.style.top = `${e.offsetY - 10}px`;
            this._tooltip.style.display = 'block';
            this.renderer.domElement.style.cursor = 'pointer';
        } else {
            this._tooltip.style.display = 'none';
            this.renderer.domElement.style.cursor = '';
        }
    }

    _toThree(wp) {
        return new THREE.Vector3(wp.x, wp.z, -wp.y);
    }

    update(waypoints) {
        if (!waypoints.length) {
            if (this.initialized && this.pathGroup) clearObject3DGroup(this.pathGroup);
            return;
        }

        this._init();
        clearObject3DGroup(this.pathGroup);
        this._waypointMeshes = [];

        const pts = waypoints.map((wp) => this._toThree(wp));

        const lineGeo = new THREE.BufferGeometry().setFromPoints(pts);
        this.pathGroup.add(
            new THREE.Line(lineGeo, new THREE.LineBasicMaterial({ color: 0x14b8a6 }))
        );

        const box = new THREE.Box3().setFromPoints(pts);
        const center = box.getCenter(new THREE.Vector3());
        const size = box.getSize(new THREE.Vector3());
        const span = Math.max(size.x, size.y, size.z, 20);

        const radius = Math.max(span * 0.025, 0.5);
        const sphereGeo = new THREE.SphereGeometry(radius, 14, 14);

        pts.forEach((pt, i) => {
            const color =
                i === 0 ? 0x14b8a6 : i === pts.length - 1 ? 0xd97706 : 0x4a7bc0;
            const mesh = new THREE.Mesh(
                sphereGeo,
                new THREE.MeshPhongMaterial({ color })
            );
            mesh.position.copy(pt);
            mesh.userData = { stepIndex: i, waypoint: waypoints[i] };
            this.pathGroup.add(mesh);
            this._waypointMeshes.push(mesh);
        });

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
// Dashboard
// ---------------------------------------------------------------------------

const PANEL_PRESET_KEY = {
    'preset-hover': 'hover',
    'preset-circular': 'circular',
    'preset-step': 'step',
    'preset-trapezoidal': 'trapezoidal',
};

const PRESET_JSON_URL = {
    hover: '/tests/generateTests/JSONtests/hover_test.json',
    circular: '/tests/generateTests/JSONtests/circular_test.json',
    step: '/tests/generateTests/JSONtests/step_test.json',
    trapezoidal: '/tests/generateTests/JSONtests/trapezoidal_test.json',
};

function presetJsonFetchUrl(path) {
    const p = path.startsWith('/') ? path : `/${path}`;
    return new URL(p, window.location.origin).href;
}

const PRESET_VIZ_IDS = {
    hover: 'viz-hover',
    circular: 'viz-circular',
    step: 'viz-step',
    trapezoidal: 'viz-trapezoidal',
};

const PRESET_STATUS_IDS = {
    hover: 'presetStatusHover',
    circular: 'presetStatusCircular',
    step: 'presetStatusStep',
    trapezoidal: 'presetStatusTrapezoidal',
};

const PRESET_RESULT_IDS = {
    hover: 'rcResultHover',
    circular: 'rcResultCircular',
    step: 'rcResultStep',
    trapezoidal: 'rcResultTrapezoidal',
};

const PRESET_META_IDS = {
    hover: 'rcMetaHover',
    circular: 'rcMetaCircular',
    step: 'rcMetaStep',
    trapezoidal: 'rcMetaTrapezoidal',
};

class TestAutomationDashboard {
    constructor() {
        this.runButton = document.querySelector('.search-section .run-btn');
        this.logTableBody = document.getElementById('logTableBody');
        this.navSubLinks = document.querySelectorAll('.nav-sub-link');
        this.waypoints = [];
        this.viz = new Visualizer3D('viz3d');

        this.presetViz = {};
        this._presetLoaded = {};
        this._presetLoading = {};
        this._presetRcCommands = {};
        this._lastRcCommands = null;

        this.initializeEventListeners();
        this.setupWebSocket();
        this.initCustomInput();
        this.initWorkspaceNav();
        this.initPresetSendButtons();
    }

    initWorkspaceNav() {
        this.navSubLinks.forEach((btn) => {
            btn.addEventListener('click', () => {
                const panel = btn.getAttribute('data-panel');
                if (!panel) return;
                this.showWorkspacePanel(panel);
            });
        });
    }

    showWorkspacePanel(panelId) {
        this.navSubLinks.forEach((b) => {
            b.classList.toggle('active', b.getAttribute('data-panel') === panelId);
        });
        document.querySelectorAll('.workspace-panel').forEach((p) => {
            p.classList.toggle('active', p.id === `panel-${panelId}`);
        });

        const presetKey = PANEL_PRESET_KEY[panelId];
        if (presetKey) {
            requestAnimationFrame(() => {
                requestAnimationFrame(() => this.loadPresetIfNeeded(presetKey));
            });
        }
    }

    initPresetSendButtons() {
        document.querySelectorAll('[data-send-preset]').forEach((btn) => {
            btn.addEventListener('click', () => {
                const key = btn.getAttribute('data-send-preset');
                this.sendPresetRcToServer(key);
            });
        });
    }

    sendPresetRcToServer(presetKey) {
        const cmds = this._presetRcCommands[presetKey];
        if (!cmds || !cmds.length) {
            alert('No RC commands loaded for this test yet.');
            return;
        }
        const payload = { command: 'rcCommands', commands: cmds };
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify(payload));
            const ts = new Date().toLocaleTimeString('en-US', { hour12: false });
            this.addLogEntry(
                ts,
                'info',
                'preset-test',
                `Sent ${cmds.length} RC commands (${presetKey}) to server`
            );
        } else {
            alert('Not connected to test server.');
        }
    }

    async loadPresetIfNeeded(presetKey) {
        if (this._presetLoaded[presetKey] || this._presetLoading[presetKey]) return;
        this._presetLoading[presetKey] = true;

        const statusEl = document.getElementById(PRESET_STATUS_IDS[presetKey]);
        const resultEl = document.getElementById(PRESET_RESULT_IDS[presetKey]);
        const metaEl = document.getElementById(PRESET_META_IDS[presetKey]);

        if (statusEl) {
            statusEl.textContent = 'Loading JSON';
            statusEl.classList.remove('preset-error');
        }

        try {
            const path = PRESET_JSON_URL[presetKey];
            if (!path || !path.startsWith('/')) {
                throw new Error('Preset URL must be a root-relative path starting with /');
            }
            const url = presetJsonFetchUrl(path);
            console.log('Attempting to load flight data from:', url);

            const res = await fetch(path);
            if (!res.ok) {
                console.error(
                    'Preset JSON HTTP error:',
                    res.status,
                    res.statusText,
                    'url:',
                    url
                );
                throw new Error(`${res.status} ${res.statusText} (${url})`);
            }
            const data = await res.json();
            if (!Array.isArray(data) || data.length < 2) {
                throw new Error('JSON must be an array with at least two samples.');
            }

            const waypoints = jsonRecordsToWaypoints(data);
            const cmdList = genericConvertToCommandList(data);
            const displayStr = formatGenericRcDisplay(cmdList);

            requestAnimationFrame(() => {
                if (!this.presetViz[presetKey]) {
                    this.presetViz[presetKey] = new Visualizer3D(PRESET_VIZ_IDS[presetKey]);
                }
                this.presetViz[presetKey].update(waypoints);
            });

            if (resultEl) resultEl.textContent = displayStr || '(no movement)';
            if (metaEl) {
                metaEl.textContent = `JSON samples: ${data.length}. Command tokens: ${cmdList.length}. Display adds a dot after each R, L, F, and B, uses I. for idle, and leaves U and D as single letters (same rules as generic_conversion.py).`;
            }
            this._presetRcCommands[presetKey] = cmdList;
            this._presetLoaded[presetKey] = true;

            if (statusEl) {
                statusEl.textContent = `Loaded ${data.length} samples from preset JSON.`;
            }
        } catch (e) {
            console.error('Preset load failed:', presetKey, e);
            if (statusEl) {
                statusEl.textContent = `Load failed: ${e.message}`;
                statusEl.classList.add('preset-error');
            }
            if (resultEl) resultEl.textContent = '';
            if (metaEl) metaEl.textContent = '';
        } finally {
            this._presetLoading[presetKey] = false;
        }
    }

    initializeEventListeners() {
        if (this.runButton) {
            this.runButton.addEventListener('click', () => this.runTest());
        }
    }

    initCustomInput() {
        document.querySelectorAll('.input-tab-btn').forEach((btn) => {
            btn.addEventListener('click', () => {
                document.querySelectorAll('.input-tab-btn').forEach((b) => b.classList.remove('active'));
                document.querySelectorAll('.input-tab-content').forEach((c) => c.classList.remove('active'));
                btn.classList.add('active');
                document.getElementById(`tab-${btn.dataset.tab}`).classList.add('active');
            });
        });

        document.getElementById('addWaypointBtn').addEventListener('click', () => this.addWaypoint());
        document.getElementById('clearWaypointsBtn').addEventListener('click', () => this.clearWaypoints());
        document.getElementById('convertWaypointsBtn').addEventListener('click', () => this.convertWaypointsToRc());

        document.getElementById('convertRawBtn').addEventListener('click', () => this.convertRawToRc());

        document.getElementById('sendToServerBtn').addEventListener('click', () => this.sendRcToServer());
    }

    addWaypoint() {
        const x = parseFloat(document.getElementById('wp-x').value) || 0;
        const y = parseFloat(document.getElementById('wp-y').value) || 0;
        const z = parseFloat(document.getElementById('wp-z').value) || 0;

        this.waypoints.push({ x, y, z });
        this.renderWaypointsTable();

        ['wp-x', 'wp-y', 'wp-z'].forEach((id) => {
            document.getElementById(id).value = '';
        });
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

        tbody.querySelectorAll('.remove-wp-btn').forEach((btn) => {
            btn.addEventListener('click', () => {
                this.waypoints.splice(parseInt(btn.dataset.idx, 10), 1);
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
        if (!text) {
            alert('Paste data first.');
            return;
        }

        try {
            let waypoints;
            if (format === 'json') {
                const data = JSON.parse(text);
                waypoints = data.map((r) => ({
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
            alert(`Parse error: ${e.message}`);
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
            this.addLogEntry(
                ts,
                'info',
                'user-input',
                `Sent ${this._lastRcCommands.length} RC commands to server`
            );
        } else {
            alert('Not connected to test server.');
        }
    }

    setupWebSocket() {
        try {
            this.ws = new WebSocket('ws://localhost:8080/ws');

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
                setTimeout(() => this.setupWebSocket(), 5000);
            };
        } catch (error) {
            console.log('WebSocket connection failed, running in standalone mode');
        }
    }

    runTest() {
        const timestamp = new Date().toLocaleTimeString('en-US', { hour12: false });

        this.addLogEntry(timestamp, 'info', 'setup', 'Test execution started');

        setTimeout(() => {
            this.addLogEntry(timestamp, 'info', 'execute', 'Running test cases...');
        }, 1000);

        setTimeout(() => {
            this.addLogEntry(timestamp, 'warn', 'execute', 'Network latency detected');
        }, 2000);

        setTimeout(() => {
            this.addLogEntry(timestamp, 'info', 'teardown', 'Test completed successfully');
        }, 3000);

        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(
                JSON.stringify({
                    command: 'runTest',
                    testId: 'search-test',
                })
            );
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
            default:
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

        const logsSection = document.querySelector('.logs-section');
        if (logsSection) {
            logsSection.scrollTop = logsSection.scrollHeight;
        }
    }
}

document.addEventListener('DOMContentLoaded', () => {
    window.dashboard = new TestAutomationDashboard();
});
