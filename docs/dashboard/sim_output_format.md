# Sim output format (Flight Sim → Dashboard)

The dashboard shows what the drone *actually* did during a test by reading JSON
files written by flight sim. This page is the format those files should use.

Status: **proposal**. Items marked *open* still need a decision with FS.

## Where files go

Write one `.json` file per run into the dashboard's sim output folder:

- Default: `sim_output/` at the repo root (ignored by git).
- Override: set `SIM_OUTPUT_DIR` for the dashboard server (e.g. in
  `slayterhil-dashboard.service`).

The dashboard lists the folder (newest first) on every test panel and
refreshes every 5 s. File names: letters, digits, `_`, `-`, `.`, ending in
`.json` (e.g. `hover-20261115-143012.json`).

*Open:* where the sim will actually write these on the Pi.

## File shape

Any of these three is accepted:

```jsonc
// 1. Array of records (same shape as the test JSON in tests/generateTests/JSONtests/)
[ { ...record }, { ...record } ]

// 2. One record
{ ...record }

// 3. Batch with a run ID (preferred)
{ "run_id": "hover-20261115-143012", "samples": [ { ...record }, ... ] }
```

## Record fields

Same names as the reference test JSON, so actual vs reference is a direct
field-by-field comparison.

| Field | Required | Type | Meaning |
|---|---|---|---|
| `Timestamp` | yes | number | Seconds since test start (sim time, not wall clock) |
| `X_pos`, `Y_pos`, `Z_pos` | yes | number | Simulated position, m |
| `X_vel_ext`, `Y_vel_ext`, `Z_vel_ext` | recommended | number | Simulated velocity, m/s. All three or none |
| `Message_id` | recommended | integer | Increments by 1 per packet. Used to flag missing, duplicated or out-of-order packets |
| `run_id` | recommended | string | Run this data belongs to (can be on the batch instead of each record) |
| `quat` | optional | `[w, x, y, z]` | Orientation as a unit quaternion, body → world |

Frame: **X right, Y forward, Z up**, meters and seconds. That matches the
reference files and `flight_sim` (gravity is −Z).

Rate: anything works; ~20 samples/s is plenty for the view. The sim runs
at 60 Hz, so writing every 3rd step is fine.

### Run ID

When **Start Test** is pressed the dashboard creates a run ID such as
`hover-20261115-143012` and sends it with the RC commands
(`{"command": "rcCommands", "commands": [...], "run_id": "..."}`).
If the output file carries a `run_id` that doesn't match the last run
started on that panel, the dashboard shows a warning. With no `run_id` it
still loads, marked "run not identified".

*Open:* how the sim learns the run ID (currently it reads `test.txt`).

### Orientation

*Open:* the sim doesn't compute orientation yet (`Drone::orientation` stays
identity and no torque is applied). When it does, send a quaternion rather
than roll/pitch/yaw so there is no rotation-order ambiguity. The dashboard
tilts the drone model with it and shows tilt from vertical and heading
(0° = +Y, positive toward +X). Angular velocity can be added as
`X_angvel`, `Y_angvel`, `Z_angvel` (deg/s, body frame) once defined.

## Example

```json
{
  "run_id": "hover-20261115-143012",
  "samples": [
    { "Message_id": 1, "Timestamp": 0.0, "X_pos": 0.0, "Y_pos": 0.0, "Z_pos": 0.0,
      "X_vel_ext": 0.0, "Y_vel_ext": 0.0, "Z_vel_ext": 0.0 },
    { "Message_id": 2, "Timestamp": 0.05, "X_pos": 0.0, "Y_pos": 0.0, "Z_pos": 0.03,
      "X_vel_ext": 0.0, "Y_vel_ext": 0.0, "Z_vel_ext": 1.2,
      "quat": [1.0, 0.0, 0.0, 0.0] }
  ]
}
```

## What the dashboard computes

- Reference position/velocity at each sample's `Timestamp`, linearly
  interpolated between reference points (which are 0.5 s apart).
- Error = actual − reference, per axis; RMS per axis and max |error| for
  the run.
- Error is only computed when the reference has timestamps (the presets do;
  hand-entered waypoints don't).

## Testing without the sim

`tests/mock_sim_output.py` writes **fake** output in this format (a point
dragged toward the reference by a spring, plus noise):

```bash
python3 tests/mock_sim_output.py hover                 # → sim_output/hover_mock_output.json
python3 tests/mock_sim_output.py circular --tilt       # adds a fake quaternion
python3 tests/mock_sim_output.py step --drop 3         # drops packets to test the warning
python3 tests/mock_sim_output.py hover --run-id hover-20261115-143012
```

## Known mismatch to resolve first

Actual vs reference error is only meaningful once TA, FS and test node
agree what one RC command means. Today the dashboard converts paths at
7.5 m per command (presets) or 0.5 m (custom input). The sim treats each
command as a ±1 m target from the origin, held for 0.5 s
(`flight_sim/src/main.cpp`).
