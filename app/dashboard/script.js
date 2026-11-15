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
                    if (dx > 0) commands.push('R');
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

/**
 * Apply the RC delimiter rules to a raw token list (R/L/F/B/U/D/I):
 * '.' after R, L, F, B and I; U and D stay bare; trailing 'I.' if the
 * sequence ends on U or D. Tokens that already carry a '.' are left alone.
 * This is what gets displayed AND what gets sent to the test node.
 */
function toDelimitedTokens(commandList) {
    const tokens = commandList.map((c) => {
        if (c.endsWith('.')) return c;
        if (c === 'R' || c === 'L' || c === 'F' || c === 'B' || c === 'I') return `${c}.`;
        return c;
    });
    const last = tokens[tokens.length - 1];
    if (last === 'U' || last === 'D') tokens.push('I.');
    return tokens;
}

function jsonRecordsToWaypoints(data) {
    // t / vx / vy / vz are kept so sim output can be compared against the
    // reference over time. They stay undefined for inputs that lack them.
    return data.map((r) => ({
        x: r.X_pos ?? r.x ?? 0,
        y: r.Y_pos ?? r.y ?? 0,
        z: r.Z_pos ?? r.z ?? 0,
        t: r.Timestamp,
        vx: r.X_vel_ext,
        vy: r.Y_vel_ext,
        vz: r.Z_vel_ext,
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
// Sim output (actual flight data from flight sim)
// Format: docs/dashboard/sim_output_format.md
// ---------------------------------------------------------------------------

function finiteOrNaN(v) {
    return typeof v === 'number' && Number.isFinite(v) ? v : NaN;
}

/**
 * Normalize a sim output file into { runId, samples }. Accepts:
 *   - an array of records (same shape as the reference test JSON)
 *   - a single record
 *   - { run_id, samples: [records] }
 * Each sample: { seq, t, x, y, z, vx, vy, vz, q } with v* / q null when absent.
 * q is a unit quaternion [w, x, y, z] in the sim frame (Z up).
 */
function parseSimOutput(raw) {
    let list;
    let runId = null;
    if (Array.isArray(raw)) {
        list = raw;
    } else if (raw && typeof raw === 'object' && Array.isArray(raw.samples)) {
        list = raw.samples;
        runId = raw.run_id ?? null;
    } else if (raw && typeof raw === 'object' && 'Timestamp' in raw) {
        list = [raw];
    } else {
        throw new Error('Expected an array of records, a single record, or {"run_id", "samples"}.');
    }
    if (!list.length) throw new Error('File contains no samples.');

    const samples = list.map((r, i) => {
        if (!r || typeof r !== 'object') throw new Error(`Sample ${i + 1} is not an object.`);
        const t = finiteOrNaN(r.Timestamp);
        const x = finiteOrNaN(r.X_pos);
        const y = finiteOrNaN(r.Y_pos);
        const z = finiteOrNaN(r.Z_pos);
        if (![t, x, y, z].every(Number.isFinite)) {
            throw new Error(`Sample ${i + 1}: Timestamp, X_pos, Y_pos and Z_pos must be numbers.`);
        }
        const vx = finiteOrNaN(r.X_vel_ext);
        const vy = finiteOrNaN(r.Y_vel_ext);
        const vz = finiteOrNaN(r.Z_vel_ext);
        const hasVel = [vx, vy, vz].every(Number.isFinite);

        let q = null;
        if (Array.isArray(r.quat) && r.quat.length === 4 && r.quat.every((c) => Number.isFinite(c))) {
            const n = Math.hypot(...r.quat);
            if (n > 1e-9) q = r.quat.map((c) => c / n);
        }

        if (runId === null && typeof r.run_id === 'string') runId = r.run_id;
        const seq = finiteOrNaN(r.Message_id);
        return {
            seq: Number.isFinite(seq) ? seq : null,
            t,
            x,
            y,
            z,
            vx: hasVel ? vx : null,
            vy: hasVel ? vy : null,
            vz: hasVel ? vz : null,
            q,
        };
    });

    const packetCheck = checkPackets(samples); // before sorting: file order matters
    samples.sort((a, b) => a.t - b.t);
    return { runId, samples, packetCheck };
}

/** Missing / duplicated / out-of-order Message_ids, in file order (only when every sample has one). */
function checkPackets(samples) {
    if (!samples.every((s) => s.seq !== null)) return null;
    const seen = new Set();
    let duplicates = 0;
    let outOfOrder = 0;
    let prev = -Infinity;
    samples.forEach((s) => {
        if (seen.has(s.seq)) duplicates += 1;
        seen.add(s.seq);
        if (s.seq < prev) outOfOrder += 1;
        prev = Math.max(prev, s.seq);
    });
    const ids = [...seen];
    const span = Math.max(...ids) - Math.min(...ids) + 1;
    return { missing: span - ids.length, duplicates, outOfOrder };
}

/** Binary search: last index with arr[i].t <= t (arr sorted by t, length >= 1). */
function indexAtTime(arr, t) {
    let lo = 0;
    let hi = arr.length - 1;
    if (t <= arr[0].t) return 0;
    if (t >= arr[hi].t) return hi;
    while (hi - lo > 1) {
        const mid = (lo + hi) >> 1;
        if (arr[mid].t <= t) lo = mid;
        else hi = mid;
    }
    return lo;
}

/** Linear interpolation of the given keys at time t; clamps outside the range. */
function interpolateAt(arr, t, keys) {
    const i = indexAtTime(arr, t);
    const a = arr[i];
    const b = arr[Math.min(i + 1, arr.length - 1)];
    const span = b.t - a.t;
    const f = span > 0 ? Math.min(Math.max((t - a.t) / span, 0), 1) : 0;
    const out = { idx: i };
    keys.forEach((k) => {
        const av = a[k];
        const bv = b[k];
        out[k] = Number.isFinite(av) && Number.isFinite(bv) ? av + (bv - av) * f : null;
    });
    return out;
}

/** Reference waypoints usable for time alignment (needs a numeric t on every point). */
function timedReference(waypoints) {
    if (!waypoints || !waypoints.length) return null;
    if (!waypoints.every((w) => Number.isFinite(w.t))) return null;
    return [...waypoints].sort((a, b) => a.t - b.t);
}

/** Error stats (actual − reference), position only. Null when no timed reference. */
function computeErrorStats(samples, ref) {
    if (!ref || !samples.length) return null;
    const sq = { x: 0, y: 0, z: 0 };
    let maxErr = -1;
    let maxAt = 0;
    samples.forEach((s) => {
        const r = interpolateAt(ref, s.t, ['x', 'y', 'z']);
        const ex = s.x - r.x;
        const ey = s.y - r.y;
        const ez = s.z - r.z;
        sq.x += ex * ex;
        sq.y += ey * ey;
        sq.z += ez * ez;
        const mag = Math.hypot(ex, ey, ez);
        if (mag > maxErr) {
            maxErr = mag;
            maxAt = s.t;
        }
    });
    const n = samples.length;
    return {
        rms: { x: Math.sqrt(sq.x / n), y: Math.sqrt(sq.y / n), z: Math.sqrt(sq.z / n) },
        max: maxErr,
        maxAt,
    };
}

/** Sim-frame (Z up) quaternion [w,x,y,z] → tilt from vertical and heading, in degrees. */
function tiltAndHeading(q) {
    const [w, x, y, z] = q;
    // Body up (0,0,1) and body forward (0,1,0) rotated into the world frame.
    const upZ = 1 - 2 * (x * x + y * y);
    const fwdX = 2 * (x * y - w * z);
    const fwdY = 1 - 2 * (x * x + z * z);
    const tilt = (Math.acos(Math.min(Math.max(upZ, -1), 1)) * 180) / Math.PI;
    // Heading: 0° = +Y (forward), positive toward +X (right).
    const heading = (Math.atan2(fwdX, fwdY) * 180) / Math.PI;
    return { tilt, heading };
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

const VIZ_COLORS = {
    actual: 0xa78bfa,
    velocity: 0xfacc15,
    error: 0xf5f5f5,
    droneBody: 0x9ca3af,
    droneFront: 0x14b8a6,
    droneRear: 0xd97706,
};

class Visualizer3D {
    constructor(containerId) {
        this.container = document.getElementById(containerId);
        this.initialized = false;
        this._scale = 150;
        this._labelObjects = [];
        this.pathGroup = null;
        this.referenceWaypoints = [];
        this.onReferenceChange = null;
        this.layerVisible = {
            planned: true,
            actual: true,
            drone: true,
            velocity: true,
            error: true,
        };
        this._refPts = [];
        this._actualPts = [];
        this._span = 20;
        this._maxSpeed = 0;
        this._fitBox = null;
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

        // The panel may be hidden (0×0) when data first arrives; _onResize fixes it on show.
        const w = this.container.clientWidth || 600;
        const h = this.container.clientHeight || 380;
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
        // CSS2DRenderer gives each label its own z-index; contain them in one
        // stacking context so the hover tooltip stays on top.
        this.labelRenderer.domElement.style.zIndex = '1';
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

        this.actualGroup = new THREE.Group();
        this.scene.add(this.actualGroup);
        this._buildMarkers();
        this._applyLayerVisibility();

        window.addEventListener('resize', () => this._onResize());
        this._animate();
    }

    _buildMarkers() {
        // Drone: X-frame quad in the three.js horizontal (XZ) plane, unit size.
        // Sim +Y (forward) maps to three −Z, so the front motors sit at −Z.
        const drone = new THREE.Group();
        const bodyMat = new THREE.MeshPhongMaterial({ color: VIZ_COLORS.droneBody });
        const hub = new THREE.Mesh(new THREE.SphereGeometry(0.3, 16, 10), bodyMat);
        hub.scale.set(1, 0.45, 1);
        drone.add(hub);
        [45, -45].forEach((deg) => {
            const arm = new THREE.Mesh(new THREE.BoxGeometry(2.4, 0.08, 0.12), bodyMat);
            arm.rotation.y = (deg * Math.PI) / 180;
            drone.add(arm);
        });
        const d = 0.85;
        const frontMat = new THREE.MeshPhongMaterial({ color: VIZ_COLORS.droneFront });
        const rearMat = new THREE.MeshPhongMaterial({ color: VIZ_COLORS.droneRear });
        const motorGeo = new THREE.CylinderGeometry(0.22, 0.22, 0.1, 16);
        [
            [-d, -d, frontMat],
            [d, -d, frontMat],
            [-d, d, rearMat],
            [d, d, rearMat],
        ].forEach(([x, z, mat]) => {
            const m = new THREE.Mesh(motorGeo, mat);
            m.position.set(x, 0.06, z);
            drone.add(m);
        });
        drone.visible = false;
        drone.userData.hasData = false;
        this.scene.add(drone);
        this._drone = drone;

        const arrow = new THREE.ArrowHelper(
            new THREE.Vector3(1, 0, 0),
            new THREE.Vector3(),
            1,
            VIZ_COLORS.velocity
        );
        arrow.visible = false;
        arrow.userData.hasData = false;
        this.scene.add(arrow);
        this._velArrow = arrow;

        const errGeo = new THREE.BufferGeometry().setFromPoints([new THREE.Vector3(), new THREE.Vector3()]);
        const errLine = new THREE.Line(
            errGeo,
            new THREE.LineDashedMaterial({ color: VIZ_COLORS.error, dashSize: 1, gapSize: 0.6 })
        );
        errLine.visible = false;
        errLine.userData.hasData = false;
        this.scene.add(errLine);
        this._errLine = errLine;
    }

    setLayerVisible(name, on) {
        if (!(name in this.layerVisible)) return;
        this.layerVisible[name] = on;
        this._applyLayerVisibility();
    }

    _applyLayerVisibility() {
        if (!this.initialized) return;
        const L = this.layerVisible;
        this.pathGroup.visible = L.planned;
        this.actualGroup.visible = L.actual;
        this._drone.visible = L.drone && this._drone.userData.hasData;
        this._velArrow.visible = L.velocity && this._velArrow.userData.hasData;
        this._errLine.visible = L.error && this._errLine.userData.hasData;
    }

    /**
     * Draw the actual flown path. refit: true always refits the camera,
     * 'auto' only when the path leaves the current view box.
     */
    setActual(samples, { refit = true } = {}) {
        this._init();
        clearObject3DGroup(this.actualGroup);
        this._traversed = null;
        this._actualPts = samples.map((s) => this._toThree(s));
        this._maxSpeed = samples.reduce(
            (m, s) => (s.vx === null ? m : Math.max(m, Math.hypot(s.vx, s.vy, s.vz))),
            0
        );

        if (this._actualPts.length) {
            const faint = new THREE.Line(
                new THREE.BufferGeometry().setFromPoints(this._actualPts),
                new THREE.LineBasicMaterial({ color: VIZ_COLORS.actual, transparent: true, opacity: 0.25 })
            );
            this._traversed = new THREE.Line(
                new THREE.BufferGeometry().setFromPoints(this._actualPts),
                new THREE.LineBasicMaterial({ color: VIZ_COLORS.actual })
            );
            this.actualGroup.add(faint, this._traversed);
        }

        const outside =
            !this._fitBox || this._actualPts.some((p) => !this._fitBox.containsPoint(p));
        if (refit === true || (refit === 'auto' && outside)) this._fit();
    }

    clearActual() {
        if (!this.initialized) return;
        clearObject3DGroup(this.actualGroup);
        this._traversed = null;
        this._actualPts = [];
        this.setPlayhead(null);
        this._fit();
    }

    /** state: { idx, x, y, z, vx, vy, vz, q, ref: {x,y,z} | null } or null to hide markers. */
    setPlayhead(state) {
        if (!this.initialized) return;
        const hide = () => {
            this._drone.userData.hasData = false;
            this._velArrow.userData.hasData = false;
            this._errLine.userData.hasData = false;
        };
        if (!state) {
            hide();
            if (this._traversed) this._traversed.geometry.setDrawRange(0, Infinity);
            this._applyLayerVisibility();
            return;
        }

        const p = this._toThree(state);
        const size = this._span * 0.05;

        this._drone.position.copy(p);
        this._drone.scale.setScalar(size);
        if (state.q) {
            // Sim frame (x, y, z) → three frame (x, z, −y); rotation is proper, so
            // the quaternion vector part maps the same way.
            const [w, x, y, z] = state.q;
            this._drone.quaternion.set(x, z, -y, w);
        } else {
            this._drone.quaternion.identity();
        }
        this._drone.userData.hasData = true;

        if (this._traversed) this._traversed.geometry.setDrawRange(0, state.idx + 1);

        const speed = state.vx === null ? 0 : Math.hypot(state.vx, state.vy, state.vz);
        if (speed > 1e-6 && this._maxSpeed > 0) {
            const dir = new THREE.Vector3(state.vx, state.vz, -state.vy).normalize();
            const len = Math.max((speed / this._maxSpeed) * this._span * 0.2, size * 1.5);
            this._velArrow.position.copy(p);
            this._velArrow.setDirection(dir);
            this._velArrow.setLength(len, len * 0.25, len * 0.15);
            this._velArrow.userData.hasData = true;
        } else {
            this._velArrow.userData.hasData = false;
        }

        if (state.ref) {
            const pos = this._errLine.geometry.attributes.position;
            const r = this._toThree(state.ref);
            pos.setXYZ(0, p.x, p.y, p.z);
            pos.setXYZ(1, r.x, r.y, r.z);
            pos.needsUpdate = true;
            this._errLine.geometry.computeBoundingSphere();
            this._errLine.computeLineDistances();
            this._errLine.userData.hasData = true;
        } else {
            this._errLine.userData.hasData = false;
        }

        this._applyLayerVisibility();
    }

    /** Fit the camera to the planned + actual paths. */
    _fit() {
        if (!this.initialized) return;
        const all = [...this._refPts, ...this._actualPts];
        if (!all.length) return;
        const box = new THREE.Box3().setFromPoints(all);
        const center = box.getCenter(new THREE.Vector3());
        const size = box.getSize(new THREE.Vector3());
        const span = Math.max(size.x, size.y, size.z, 20);
        this._span = span;
        this._fitBox = box.clone().expandByScalar(span * 0.1);

        this._errLine.material.dashSize = span * 0.012;
        this._errLine.material.gapSize = span * 0.008;

        this._scale = span * 1.0;
        this._updateFrustum();
        this._rebuildAxisLabels();
        this.controls.target.copy(center);
        this.controls.update();
    }

    _updateFrustum() {
        const aspect = (this.container.clientWidth || 600) / (this.container.clientHeight || 380);
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
        this.referenceWaypoints = waypoints;
        if (!waypoints.length) {
            this._refPts = [];
            if (this.initialized && this.pathGroup) {
                clearObject3DGroup(this.pathGroup);
                this._waypointMeshes = [];
                this._fit();
            }
            if (this.onReferenceChange) this.onReferenceChange();
            return;
        }

        this._init();
        clearObject3DGroup(this.pathGroup);
        this._waypointMeshes = [];

        const pts = waypoints.map((wp) => this._toThree(wp));
        this._refPts = pts;

        const lineGeo = new THREE.BufferGeometry().setFromPoints(pts);
        this.pathGroup.add(
            new THREE.Line(lineGeo, new THREE.LineBasicMaterial({ color: 0x14b8a6 }))
        );

        const size = new THREE.Box3().setFromPoints(pts).getSize(new THREE.Vector3());
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

        this._fit();
        if (this.onReferenceChange) this.onReferenceChange();
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
        if (!w || !h) return; // hidden panel; resized again when shown
        this._updateFrustum();
        this.renderer.setSize(w, h);
        this.labelRenderer.setSize(w, h);
    }
}

// ---------------------------------------------------------------------------
// Sim output panel (file picker, layer toggles, playback, readout, stats)
// ---------------------------------------------------------------------------

const LAYER_DEFS = [
    { key: 'planned', label: 'Planned path', color: '#14b8a6' },
    { key: 'actual', label: 'Actual path', color: '#a78bfa' },
    { key: 'drone', label: 'Drone', color: '#9ca3af' },
    { key: 'velocity', label: 'Velocity', color: '#facc15' },
    { key: 'error', label: 'Error line', color: '#f5f5f5' },
    { key: 'readout', label: 'Readout', color: null },
];

const LAYER_PREFS_KEY = 'slayterhil.viz.layers';
const LAYER_PREFS_EVENT = 'slayterhil:layers-changed';
const PLAYBACK_SPEEDS = [0.25, 0.5, 1, 2, 4];

function loadLayerPrefs() {
    try {
        const p = JSON.parse(localStorage.getItem(LAYER_PREFS_KEY));
        return p && typeof p === 'object' ? p : {};
    } catch {
        return {};
    }
}

function saveLayerPrefs(prefs) {
    try {
        localStorage.setItem(LAYER_PREFS_KEY, JSON.stringify(prefs));
    } catch {
        // storage unavailable (private window etc.): toggles still work for this page
    }
}

function fmtNum(v, digits = 2) {
    return v === null || v === undefined || !Number.isFinite(v) ? '—' : v.toFixed(digits);
}

function escapeHtml(s) {
    return String(s).replace(/[&<>"']/g, (c) => ({
        '&': '&amp;',
        '<': '&lt;',
        '>': '&gt;',
        '"': '&quot;',
        "'": '&#39;',
    }[c]));
}

class RunOutputView {
    /**
     * @param {Visualizer3D} viz
     * @param {{ onRefreshFiles?: () => void }} opts
     */
    constructor(viz, { onRefreshFiles } = {}) {
        this.viz = viz;
        this.onRefreshFiles = onRefreshFiles;
        this.run = null; // { runId, samples, packetCheck, label }
        this.ref = null; // timed reference or null
        this.stats = null;
        this.expectedRunId = null; // last run started from this panel
        this.t = 0;
        this.playing = false;
        this.speed = 1;
        this.showReadout = true;
        this._tick = this._tick.bind(this);

        this._buildDom();
        this._applyPrefs(loadLayerPrefs());
        window.addEventListener(LAYER_PREFS_EVENT, (e) => this._applyPrefs(e.detail));
        viz.onReferenceChange = () => this._refreshReference();
    }

    _buildDom() {
        const section = this.viz.container.closest('.viz3d-section');
        const footer = section.querySelector('.viz3d-footer');
        const root = document.createElement('div');
        root.className = 'sim-output';

        const chips = LAYER_DEFS.map(
            (l) => `
            <label class="sim-chip">
                <input type="checkbox" data-layer="${l.key}" checked>
                ${l.color ? `<span class="sim-swatch" style="background:${l.color}"></span>` : ''}
                <span>${l.label}</span>
            </label>`
        ).join('');

        const speeds = PLAYBACK_SPEEDS.map(
            (s) => `<option value="${s}"${s === 1 ? ' selected' : ''}>${s}×</option>`
        ).join('');

        root.innerHTML = `
            <div class="sim-output-bar">
                <span class="sim-output-title">Sim output</span>
                <select class="sim-file-select" aria-label="Sim output file">
                    <option value="">Loading file list…</option>
                </select>
                <button type="button" class="sim-btn" data-act="refresh" title="Refresh file list" aria-label="Refresh file list">↻</button>
                <button type="button" class="sim-btn" data-act="upload">Load from computer…</button>
                <button type="button" class="sim-btn" data-act="clear" hidden>Clear</button>
                <input type="file" accept=".json,application/json" hidden>
            </div>
            <div class="sim-status" role="status"></div>
            <div class="sim-layers" role="group" aria-label="Visible layers">${chips}</div>
            <div class="sim-playback" hidden>
                <button type="button" class="sim-btn sim-play" data-act="play" aria-label="Play">▶</button>
                <input type="range" class="sim-scrub" min="0" max="1" step="any" value="0" aria-label="Playback time">
                <span class="sim-time">0.00 / 0.00 s</span>
                <select class="sim-speed" aria-label="Playback speed">${speeds}</select>
            </div>
            <div class="sim-readout" hidden></div>
            <div class="sim-stats" hidden></div>
        `;
        section.insertBefore(root, footer);

        const $ = (sel) => root.querySelector(sel);
        this.el = {
            fileSelect: $('.sim-file-select'),
            fileInput: $('input[type="file"]'),
            clearBtn: $('[data-act="clear"]'),
            status: $('.sim-status'),
            playback: $('.sim-playback'),
            playBtn: $('[data-act="play"]'),
            scrub: $('.sim-scrub'),
            time: $('.sim-time'),
            speed: $('.sim-speed'),
            stats: $('.sim-stats'),
            readout: $('.sim-readout'),
            layerInputs: root.querySelectorAll('[data-layer]'),
        };

        this.el.fileSelect.addEventListener('change', () => {
            const name = this.el.fileSelect.value;
            if (name) this._loadServerFile(name);
        });
        $('[data-act="refresh"]').addEventListener('click', () => {
            if (this.onRefreshFiles) this.onRefreshFiles();
        });
        $('[data-act="upload"]').addEventListener('click', () => this.el.fileInput.click());
        this.el.fileInput.addEventListener('change', () => this._loadLocalFile());
        this.el.clearBtn.addEventListener('click', () => this.clear());
        this.el.playBtn.addEventListener('click', () => (this.playing ? this._pause() : this._play()));
        this.el.scrub.addEventListener('input', () => {
            this._pause();
            this._setTime(parseFloat(this.el.scrub.value));
        });
        this.el.speed.addEventListener('change', () => {
            this.speed = parseFloat(this.el.speed.value) || 1;
        });
        this.el.layerInputs.forEach((input) => {
            input.addEventListener('change', () => {
                const prefs = loadLayerPrefs();
                prefs[input.dataset.layer] = input.checked;
                saveLayerPrefs(prefs);
                // Keep every panel in sync (prefs are dashboard-wide).
                window.dispatchEvent(new CustomEvent(LAYER_PREFS_EVENT, { detail: prefs }));
            });
        });
    }

    _applyPrefs(prefs) {
        this.el.layerInputs.forEach((input) => {
            const on = prefs[input.dataset.layer] !== false;
            input.checked = on;
            if (input.dataset.layer === 'readout') this.showReadout = on;
            else this.viz.setLayerVisible(input.dataset.layer, on);
        });
        this._updateReadoutVisibility();
    }

    _updateReadoutVisibility() {
        this.el.readout.hidden = !(this.run && this.showReadout);
    }

    /** files: [{ name, size, modified }] or null when the list could not be fetched. */
    setFileList(files, info = {}) {
        const sel = this.el.fileSelect;
        const current = sel.value;
        let placeholder;
        if (!files) placeholder = 'File list unavailable (server offline?)';
        else if (!info.exists) placeholder = `No folder at ${info.dir || 'sim_output/'}`;
        else if (!files.length) placeholder = `No files in ${info.dir || 'sim_output/'}`;
        else placeholder = `Select a file (${files.length})…`;

        const opts = [`<option value="">${escapeHtml(placeholder)}</option>`];
        (files || []).forEach((f) => {
            const when = new Date(f.modified * 1000).toLocaleString('en-US', {
                month: 'short',
                day: 'numeric',
                hour: '2-digit',
                minute: '2-digit',
                hour12: false,
            });
            opts.push(
                `<option value="${escapeHtml(f.name)}">${escapeHtml(f.name)}  ·  ${escapeHtml(when)}</option>`
            );
        });
        sel.innerHTML = opts.join('');
        if (files && files.some((f) => f.name === current)) sel.value = current;
    }

    async _loadServerFile(name) {
        this._setStatus(`Loading ${escapeHtml(name)}…`);
        try {
            const res = await fetch(`/api/sim-output/${encodeURIComponent(name)}`, { cache: 'no-cache' });
            if (!res.ok) throw new Error(`${res.status} ${res.statusText}`);
            this.load(await res.json(), name);
        } catch (e) {
            this._setStatus(`Could not load ${escapeHtml(name)}: ${escapeHtml(e.message)}`, 'error');
        }
    }

    async _loadLocalFile() {
        const file = this.el.fileInput.files[0];
        this.el.fileInput.value = '';
        if (!file) return;
        try {
            this.load(JSON.parse(await file.text()), file.name);
            this.el.fileSelect.value = '';
        } catch (e) {
            this._setStatus(`Could not read ${escapeHtml(file.name)}: ${escapeHtml(e.message)}`, 'error');
        }
    }

    /** Load a parsed JSON value (any shape parseSimOutput accepts). */
    load(raw, label) {
        let parsed;
        try {
            parsed = parseSimOutput(raw);
        } catch (e) {
            this._setStatus(`Could not read ${escapeHtml(label)}: ${escapeHtml(e.message)}`, 'error');
            return;
        }
        this._pause();
        this.run = { ...parsed, label };
        const { samples } = parsed;
        this.t0 = samples[0].t;
        this.tEnd = samples[samples.length - 1].t;

        this.viz.setActual(samples, { refit: true });
        this.el.scrub.min = String(this.t0);
        this.el.scrub.max = String(this.tEnd);
        this.el.playback.hidden = false;
        this.el.stats.hidden = false;
        this.el.clearBtn.hidden = false;
        this._updateReadoutVisibility();

        this._refreshReference(); // stats, status and playhead
        this._setTime(this.t0);
    }

    clear() {
        this._pause();
        this.run = null;
        this.stats = null;
        this.viz.clearActual();
        this.el.playback.hidden = true;
        this.el.stats.hidden = true;
        this.el.clearBtn.hidden = true;
        this._updateReadoutVisibility();
        this.el.fileSelect.value = '';
        this._setStatus('');
    }

    /** Called when a run is started from this panel, so loaded output can be checked against it. */
    setExpectedRun(runId) {
        this.expectedRunId = runId;
        if (this.run) this._renderStatus();
    }

    _refreshReference() {
        this.ref = timedReference(this.viz.referenceWaypoints);
        if (!this.run) return;
        this.stats = computeErrorStats(this.run.samples, this.ref);
        this._renderStatus();
        this._renderStats();
        this._setTime(this.t);
    }

    _renderStatus() {
        const { runId, samples, packetCheck, label } = this.run;
        const duration = this.tEnd - this.t0;
        const parts = [
            `<strong>${escapeHtml(label)}</strong>`,
            runId ? `run <code>${escapeHtml(runId)}</code>` : 'run not identified',
            `${samples.length} samples, ${fmtNum(duration)} s`,
        ];
        const warnings = [];
        if (runId && this.expectedRunId && runId !== this.expectedRunId) {
            warnings.push(
                `This output is from run <code>${escapeHtml(runId)}</code>, but the last run started here was <code>${escapeHtml(this.expectedRunId)}</code>.`
            );
        }
        if (packetCheck && (packetCheck.missing || packetCheck.duplicates || packetCheck.outOfOrder)) {
            warnings.push(
                `Packets: ${packetCheck.missing} missing, ${packetCheck.duplicates} duplicated, ${packetCheck.outOfOrder} out of order (by Message_id).`
            );
        }
        if (!samples.some((s) => s.vx !== null)) warnings.push('No velocity fields; velocity arrow hidden.');
        this._setStatus(
            parts.join(' · ') + warnings.map((w) => `<div class="sim-warn">⚠ ${w}</div>`).join(''),
            ''
        );
    }

    _renderStats() {
        if (!this.stats) {
            this.el.stats.textContent = 'Error not computed: this test\'s reference path has no timestamps.';
            return;
        }
        const { rms, max, maxAt } = this.stats;
        this.el.stats.innerHTML = `
            <span>RMS error <b>X</b> ${fmtNum(rms.x)} <b>Y</b> ${fmtNum(rms.y)} <b>Z</b> ${fmtNum(rms.z)} m</span>
            <span>Max |error| ${fmtNum(max)} m at ${fmtNum(maxAt)} s</span>
            <button type="button" class="sim-link" data-act="jump-max">Jump to max</button>
        `;
        this.el.stats.querySelector('[data-act="jump-max"]').addEventListener('click', () => {
            this._pause();
            this._setTime(maxAt);
        });
    }

    _setStatus(html, kind = '') {
        this.el.status.innerHTML = html;
        this.el.status.className = `sim-status${kind ? ` sim-status-${kind}` : ''}`;
    }

    _setTime(t) {
        if (!this.run) return;
        const tc = Math.min(Math.max(Number.isFinite(t) ? t : this.t0, this.t0), this.tEnd);
        this.t = tc;
        const { samples } = this.run;
        const keys = ['x', 'y', 'z', 'vx', 'vy', 'vz'];
        const s = interpolateAt(samples, tc, keys);
        s.q = samples[s.idx].q;
        const ref = this.ref ? interpolateAt(this.ref, tc, keys) : null;

        this.viz.setPlayhead({ ...s, ref });
        this.el.readout.innerHTML = this._readoutHtml(tc, s, ref);
        this.el.scrub.value = String(tc);
        this.el.time.textContent = `${fmtNum(tc)} / ${fmtNum(this.tEnd)} s`;
    }

    _readoutHtml(t, s, ref) {
        const row = (label, x, y, z, unit, extra = '') =>
            `<tr><th>${label}</th><td>${fmtNum(x)}</td><td>${fmtNum(y)}</td><td>${fmtNum(z)}</td><td class="u">${unit}</td><td class="x">${extra}</td></tr>`;
        const rows = [
            `<tr class="h"><th>t ${fmtNum(t)} s</th><td>X</td><td>Y</td><td>Z</td><td></td><td></td></tr>`,
            row('pos', s.x, s.y, s.z, 'm'),
        ];
        if (s.vx !== null) {
            rows.push(row('vel', s.vx, s.vy, s.vz, 'm/s', `|v| ${fmtNum(Math.hypot(s.vx, s.vy, s.vz))}`));
        }
        if (ref) {
            const ex = s.x - ref.x;
            const ey = s.y - ref.y;
            const ez = s.z - ref.z;
            rows.push(row('ref', ref.x, ref.y, ref.z, 'm'));
            rows.push(row('err', ex, ey, ez, 'm', `|e| ${fmtNum(Math.hypot(ex, ey, ez))}`));
            if (s.vx !== null && ref.vx !== null) {
                rows.push(row('v err', s.vx - ref.vx, s.vy - ref.vy, s.vz - ref.vz, 'm/s'));
            }
        }
        let attitude = '';
        if (s.q) {
            const { tilt, heading } = tiltAndHeading(s.q);
            attitude = `<div class="att">tilt ${fmtNum(tilt, 1)}°  ·  heading ${fmtNum(heading, 1)}°</div>`;
        }
        return `<table>${rows.join('')}</table>${attitude}`;
    }

    _play() {
        if (!this.run) return;
        if (this.t >= this.tEnd) this._setTime(this.t0);
        this.playing = true;
        this.el.playBtn.textContent = '❚❚';
        this.el.playBtn.setAttribute('aria-label', 'Pause');
        this._last = performance.now();
        requestAnimationFrame(this._tick);
    }

    _pause() {
        this.playing = false;
        this.el.playBtn.textContent = '▶';
        this.el.playBtn.setAttribute('aria-label', 'Play');
    }

    _tick(now) {
        if (!this.playing) return;
        const dt = (now - this._last) / 1000;
        this._last = now;
        const t = this.t + dt * this.speed;
        if (t >= this.tEnd) this._pause();
        this._setTime(t);
        if (this.playing) requestAnimationFrame(this._tick);
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

const SIM_OUTPUT_POLL_MS = 5000;

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
        this._presetLoaded = {};
        this._presetLoading = {};
        this._presetRcCommands = {};
        this._lastRcCommands = null;

        // One visualizer + sim output view per panel, keyed 'custom' or the preset key.
        const refreshFiles = () => this.refreshSimOutputFiles();
        this.viz = new Visualizer3D('viz3d');
        this.presetViz = {};
        this.outputViews = {
            custom: new RunOutputView(this.viz, { onRefreshFiles: refreshFiles }),
        };
        Object.entries(PRESET_VIZ_IDS).forEach(([key, id]) => {
            this.presetViz[key] = new Visualizer3D(id);
            this.outputViews[key] = new RunOutputView(this.presetViz[key], { onRefreshFiles: refreshFiles });
        });

        this.initializeEventListeners();
        this.setupWebSocket();
        this.initCustomInput();
        this.initWorkspaceNav();
        this.initPresetSendButtons();
        this.initSimOutputPolling();
    }

    initSimOutputPolling() {
        this.refreshSimOutputFiles();
        setInterval(() => {
            if (document.visibilityState === 'visible') this.refreshSimOutputFiles();
        }, SIM_OUTPUT_POLL_MS);
    }

    async refreshSimOutputFiles() {
        let data = null;
        try {
            const res = await fetch('/api/sim-output', { cache: 'no-cache' });
            if (res.ok) data = await res.json();
        } catch {
            // server offline; views show "unavailable"
        }
        Object.values(this.outputViews).forEach((v) =>
            v.setFileList(data ? data.files : null, data || {})
        );
    }

    /** New run ID for a test started from the given panel, e.g. hover-20261115-143012. */
    startRun(panelKey) {
        const d = new Date();
        const p = (n) => String(n).padStart(2, '0');
        const runId = `${panelKey}-${d.getFullYear()}${p(d.getMonth() + 1)}${p(d.getDate())}-${p(d.getHours())}${p(d.getMinutes())}${p(d.getSeconds())}`;
        this.outputViews[panelKey].setExpectedRun(runId);
        return runId;
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
        // A visualizer may have been initialized while its panel was hidden.
        const viz = presetKey ? this.presetViz[presetKey] : this.viz;
        requestAnimationFrame(() => viz._onResize());
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
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            const runId = this.startRun(presetKey);
            this.ws.send(JSON.stringify({ command: 'rcCommands', commands: cmds, run_id: runId }));
            const ts = new Date().toLocaleTimeString('en-US', { hour12: false });
            this.addLogEntry(
                ts,
                'info',
                'preset-test',
                `Sent ${cmds.length} RC commands (${presetKey}, run ${runId}) to test node`
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
            statusEl.textContent = 'Loading…';
            statusEl.classList.remove('preset-error');
        }

        try {
            const path = PRESET_JSON_URL[presetKey];
            if (!path || !path.startsWith('/')) {
                throw new Error('Preset URL must be a root-relative path starting with /');
            }
            const url = presetJsonFetchUrl(path);

            // Revalidate every time so regenerated test JSON shows up without a hard refresh.
            const res = await fetch(path, { cache: 'no-cache' });
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
            const tokens = toDelimitedTokens(genericConvertToCommandList(data));
            const displayStr = tokens.join('');

            requestAnimationFrame(() => this.presetViz[presetKey].update(waypoints));

            if (resultEl) resultEl.textContent = displayStr || '(no movement)';
            if (metaEl) metaEl.textContent = `Total commands: ${tokens.length}`;
            this._presetRcCommands[presetKey] = tokens;
            this._presetLoaded[presetKey] = true;

            if (statusEl) statusEl.textContent = '';
        } catch (e) {
            console.error('Preset load failed:', presetKey, e);
            if (statusEl) {
                statusEl.textContent = 'Could not load this test. Make sure the dashboard server is running.';
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
                    t: r.Timestamp,
                    vx: r.X_vel_ext,
                    vy: r.Y_vel_ext,
                    vz: r.Z_vel_ext,
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
        const tokens = toDelimitedTokens(commands);
        document.getElementById('rcResult').textContent = tokens.join('') || '(no movement)';
        document.getElementById('rcMeta').textContent = `Total commands: ${tokens.length}`;
        document.getElementById('rcOutput').style.display = 'block';
        this._lastRcCommands = tokens;
    }

    sendRcToServer() {
        if (!this._lastRcCommands) return;
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            const runId = this.startRun('custom');
            this.ws.send(
                JSON.stringify({ command: 'rcCommands', commands: this._lastRcCommands, run_id: runId })
            );
            const ts = new Date().toLocaleTimeString('en-US', { hour12: false });
            this.addLogEntry(
                ts,
                'info',
                'user-input',
                `Sent ${this._lastRcCommands.length} RC commands (run ${runId}) to test node`
            );
        } else {
            alert('Not connected to test server.');
        }
    }

    setupWebSocket() {
        try {
            // Same host as the page, so this works over the tailnet (https → wss via
            // tailscale serve) and not only from a browser on the Pi.
            const proto = window.location.protocol === 'https:' ? 'wss' : 'ws';
            const host = window.location.host || 'localhost:8080';
            this.ws = new WebSocket(`${proto}://${host}/ws`);

            this.ws.onopen = () => {
                const ts = new Date().toLocaleTimeString('en-US', { hour12: false });
                this.addLogEntry(ts, 'info', 'setup', 'Connected to test server');
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
        // The log table was removed from index.html; fall back to the console.
        if (!this.logTableBody) {
            console.log(`[${time}] ${String(level).toUpperCase()} ${step}: ${message}`);
            return;
        }
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
