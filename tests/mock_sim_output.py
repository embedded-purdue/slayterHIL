"""Write FAKE flight-sim output for a preset test, so the dashboard's
"Sim output" view can be exercised before flight sim produces real files.

This is not a physics model. It drags a point toward the reference path with
a damped spring (so it lags and overshoots a little) and adds position noise.
--tilt adds a quaternion that leans the drone toward its acceleration, only so
the orientation display has something to show.

Output uses the same record fields as the test JSON in
tests/generateTests/JSONtests/ (see docs/dashboard/sim_output_format.md).

Examples:
    python3 tests/mock_sim_output.py hover
    python3 tests/mock_sim_output.py circular --tilt --run-id circular-20261115-143012
    python3 tests/mock_sim_output.py step --drop 3 --out /tmp/step_out.json
"""

import argparse
import json
import math
import os
import random

HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(HERE)
PRESETS_DIR = os.path.join(HERE, 'generateTests', 'JSONtests')
DEFAULT_OUT_DIR = os.path.join(PROJECT_ROOT, 'sim_output')
SIM_HZ = 60.0
G = 9.81


def load_reference(name_or_path):
    path = name_or_path
    if not os.path.exists(path):
        path = os.path.join(PRESETS_DIR, f'{name_or_path}_test.json')
    with open(path, encoding='utf-8') as f:
        data = json.load(f)
    ref = [(r['Timestamp'], (r['X_pos'], r['Y_pos'], r['Z_pos'])) for r in data]
    ref.sort(key=lambda p: p[0])
    return ref


def ref_at(ref, t):
    """Reference position and its slope (velocity) at t, linearly interpolated."""
    if t <= ref[0][0]:
        return ref[0][1], (0.0, 0.0, 0.0)
    if t >= ref[-1][0]:
        return ref[-1][1], (0.0, 0.0, 0.0)
    for (t0, p0), (t1, p1) in zip(ref, ref[1:]):
        if t0 <= t <= t1:
            f = (t - t0) / (t1 - t0) if t1 > t0 else 0.0
            pos = tuple(a + (b - a) * f for a, b in zip(p0, p1))
            vel = tuple((b - a) / (t1 - t0) if t1 > t0 else 0.0 for a, b in zip(p0, p1))
            return pos, vel
    return ref[-1][1], (0.0, 0.0, 0.0)


def tilt_quat(acc, max_tilt_deg=35.0):
    """Quaternion [w, x, y, z] turning body +Z toward the thrust direction (acc + g)."""
    tx, ty, tz = acc[0], acc[1], acc[2] + G
    n = math.sqrt(tx * tx + ty * ty + tz * tz) or 1.0
    tx, ty, tz = tx / n, ty / n, tz / n
    angle = min(math.acos(max(-1.0, min(1.0, tz))), math.radians(max_tilt_deg))
    ax, ay = -ty, tx  # z × thrust
    an = math.hypot(ax, ay)
    if an < 1e-9:
        return [1.0, 0.0, 0.0, 0.0]
    s = math.sin(angle / 2)
    return [math.cos(angle / 2), ax / an * s, ay / an * s, 0.0]


def simulate(ref, rate, tail, wn, zeta, noise, tilt, seed):
    rng = random.Random(seed)
    dt = 1.0 / SIM_HZ
    t_end = ref[-1][0] + tail
    every = max(1, round(SIM_HZ / rate))
    pos = list(ref[0][1])
    vel = [0.0, 0.0, 0.0]
    samples = []
    for k in range(int(round(t_end * SIM_HZ)) + 1):
        t = k * dt
        rp, rv = ref_at(ref, t)
        acc = [wn * wn * (rp[i] - pos[i]) + 2 * zeta * wn * (rv[i] - vel[i]) for i in range(3)]
        if k % every == 0:
            rec = {
                'Message_id': len(samples) + 1,
                'Timestamp': round(t, 4),
                'X_pos': round(pos[0] + rng.gauss(0, noise), 4),
                'Y_pos': round(pos[1] + rng.gauss(0, noise), 4),
                'Z_pos': round(pos[2] + rng.gauss(0, noise), 4),
                'X_vel_ext': round(vel[0], 4),
                'Y_vel_ext': round(vel[1], 4),
                'Z_vel_ext': round(vel[2], 4),
            }
            if tilt:
                rec['quat'] = [round(c, 6) for c in tilt_quat(acc)]
            samples.append(rec)
        for i in range(3):
            vel[i] += acc[i] * dt
            pos[i] += vel[i] * dt
    return samples


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('reference', help='preset name (hover, circular, step, trapezoidal) or path to a reference JSON')
    ap.add_argument('--out', help='output file (default: sim_output/<name>_mock_output.json)')
    ap.add_argument('--run-id', help='wrap output as {"run_id", "samples"} with this run ID')
    ap.add_argument('--rate', type=float, default=20.0, help='samples per second (default 20)')
    ap.add_argument('--tail', type=float, default=2.0, help='seconds to keep simulating after the reference ends')
    ap.add_argument('--wn', type=float, default=2.0, help='spring stiffness (natural frequency, rad/s)')
    ap.add_argument('--zeta', type=float, default=0.6, help='damping ratio')
    ap.add_argument('--noise', type=float, default=0.05, help='position noise std dev, m')
    ap.add_argument('--tilt', action='store_true', help='include a fake orientation quaternion')
    ap.add_argument('--drop', type=int, default=0, help='randomly drop this many packets (tests the missing-packet warning)')
    ap.add_argument('--seed', type=int, default=1)
    args = ap.parse_args()

    ref = load_reference(args.reference)
    samples = simulate(ref, args.rate, args.tail, args.wn, args.zeta, args.noise, args.tilt, args.seed)
    if args.drop:
        rng = random.Random(args.seed + 1)
        for idx in sorted(rng.sample(range(1, len(samples) - 1), args.drop), reverse=True):
            del samples[idx]

    out = args.out
    if not out:
        base = os.path.splitext(os.path.basename(args.reference))[0].removesuffix('_test')
        os.makedirs(DEFAULT_OUT_DIR, exist_ok=True)
        out = os.path.join(DEFAULT_OUT_DIR, f'{base}_mock_output.json')

    payload = {'run_id': args.run_id, 'samples': samples} if args.run_id else samples
    with open(out, 'w', encoding='utf-8') as f:
        json.dump(payload, f, indent=2)
    print(f'Wrote {len(samples)} mock samples ({samples[-1]["Timestamp"]:.2f} s) to {out}')


if __name__ == '__main__':
    main()
