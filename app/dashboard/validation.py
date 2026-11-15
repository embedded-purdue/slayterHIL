"""Trajectory validation shared by the dashboard and command-line checkers.

Times are simulation elapsed seconds; positions are metres and velocities m/s
in the reference frame. Both trajectories are linearly interpolated. No clock
shift, extrapolation, or replacement of missing measurements is performed.
"""

import argparse
from bisect import bisect_right
import json
import math
from pathlib import Path

POSITION_FIELDS = ("X_pos", "Y_pos", "Z_pos")
VELOCITY_FIELDS = ("X_vel_ext", "Y_vel_ext", "Z_vel_ext")
STATE_FIELDS = POSITION_FIELDS + VELOCITY_FIELDS
LIMIT_METRICS = {
    "position_rmse_limit_m": "position_rmse_m",
    "max_position_error_limit_m": "max_position_error_m",
    "velocity_rmse_limit_mps": "velocity_rmse_mps",
}


def finite_number(value):
    return (not isinstance(value, bool) and isinstance(value, (int, float))
            and math.isfinite(value))


def validate_samples(samples, name="samples", minimum=2):
    """Return independent records after checking required telemetry fields.

    Message_id is optional, but must be present on every record if used.
    Its sequence belongs to this trajectory, not to the reference trajectory.
    """
    if not isinstance(samples, list) or len(samples) < minimum:
        raise ValueError(f"{name} must contain at least {minimum} samples")
    result = []
    uses_ids = isinstance(samples[0], dict) and "Message_id" in samples[0]
    previous_id = None
    previous_time = None
    for index, sample in enumerate(samples):
        if not isinstance(sample, dict):
            raise ValueError(f"{name}[{index}] must be an object")
        for field in ("Timestamp",) + STATE_FIELDS:
            if not finite_number(sample.get(field)):
                raise ValueError(f"{name}[{index}].{field} must be a finite number")
        timestamp = sample["Timestamp"]
        if timestamp < 0 or (previous_time is not None and timestamp <= previous_time):
            raise ValueError(f"{name} timestamps must be nonnegative and strictly increasing")
        if ("Message_id" in sample) != uses_ids:
            raise ValueError(f"{name} must use Message_id consistently")
        if uses_ids:
            message_id = sample["Message_id"]
            if (type(message_id) is not int or message_id < 0
                    or (previous_id is not None and message_id <= previous_id)):
                raise ValueError(f"{name} Message_id must be nonnegative, unique, and increasing")
            previous_id = message_id
        result.append(dict(sample))
        previous_time = timestamp
    return result


def validate_limits(limits):
    if limits is None:
        return {}
    if not isinstance(limits, dict):
        raise ValueError("limits must be an object")
    for name, value in limits.items():
        if name not in LIMIT_METRICS or not finite_number(value) or value < 0:
            raise ValueError(f"Invalid validation limit: {name}")
    return dict(limits)


def compare_trajectory(reference, actual, *, limits=None, max_sample_gap_s=None):
    """Compare complete trajectories; omitted limits produce metrics, not a pass.
    Integrates squared piecewise-linear errors exactly over time, so unequal sampling intervals do not bias RMSE. Uses all reference and actual knots.
    """
    reference = validate_samples(reference, "reference")
    actual = validate_samples(actual, "actual")
    limits = validate_limits(limits)
    start, end = reference[0]["Timestamp"], reference[-1]["Timestamp"]
    if actual[0]["Timestamp"] > start or actual[-1]["Timestamp"] < end:
        raise ValueError("Actual samples do not cover the full reference time range")
    if max_sample_gap_s is not None:
        if not finite_number(max_sample_gap_s) or max_sample_gap_s <= 0:
            raise ValueError("max_sample_gap_s must be positive and finite")
        for left, right in zip(actual, actual[1:]):
            if (right["Timestamp"] > start and left["Timestamp"] < end
                    and right["Timestamp"] - left["Timestamp"] > max_sample_gap_s):
                raise ValueError("Actual sample gap exceeds max_sample_gap_s")

    times = sorted({start, end} | {
        sample["Timestamp"] for sample in reference + actual
        if start <= sample["Timestamp"] <= end
    })

    def interpolate(records, timestamps, time):
        index = min(max(bisect_right(timestamps, time) - 1, 0), len(records) - 2)
        left, right = records[index:index + 2]
        fraction = (time - left["Timestamp"]) / (right["Timestamp"] - left["Timestamp"])
        return [left[field] + fraction * (right[field] - left[field]) for field in STATE_FIELDS]

    reference_times = [sample["Timestamp"] for sample in reference]
    actual_times = [sample["Timestamp"] for sample in actual]
    errors = []
    details = []
    for time in times:
        expected = interpolate(reference, reference_times, time)
        measured = interpolate(actual, actual_times, time)
        delta = [value - target for value, target in zip(measured, expected)]
        errors.append(delta)
        details.append({
            "Timestamp": time,
            "reference": dict(zip(STATE_FIELDS, expected)),
            "actual": dict(zip(STATE_FIELDS, measured)),
            "position_error_m": math.hypot(*delta[:3]),
            "velocity_error_mps": math.hypot(*delta[3:]),
        })

    integrals = [0.0] * 6
    for index in range(1, len(times)):
        dt = times[index] - times[index - 1]
        for axis in range(6):
            a, b = errors[index - 1][axis], errors[index][axis]
            integrals[axis] += dt * (a * a + a * b + b * b) / 3
    duration = end - start
    metrics = {
        "position_rmse_m": math.sqrt(sum(integrals[:3]) / duration),
        "max_position_error_m": max(row["position_error_m"] for row in details),
        "final_position_error_m": details[-1]["position_error_m"],
        "velocity_rmse_mps": math.sqrt(sum(integrals[3:]) / duration),
    }
    if not all(math.isfinite(value) for value in metrics.values()):
        raise ValueError("Telemetry magnitudes exceed the supported numeric range")
    checks = {name: metrics[LIMIT_METRICS[name]] <= limit for name, limit in limits.items()}
    return {
        "data_valid": True,
        "passed": all(checks.values()) if checks else None,
        **metrics,
        "position_rmse_by_axis_m": dict(zip("XYZ", [math.sqrt(v / duration) for v in integrals[:3]])),
        "actual_samples_received": len(actual),
        "samples_compared": len(times),
        "duration_s": duration,
        "limits": limits,
        "checks": checks,
        "errors_by_sample": details,
    }


def main(argv=None):
    """CLI: python app/dashboard/validation.py reference.json actual.json."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference")
    parser.add_argument("actual", help="JSON array or {run_id, samples} packet")
    for name in LIMIT_METRICS:
        parser.add_argument("--" + name.replace("_", "-"), type=float)
    parser.add_argument("--max-sample-gap-s", type=float)
    args = parser.parse_args(argv)
    try:
        reference = json.loads(Path(args.reference).read_text(encoding="utf-8"))
        actual = json.loads(Path(args.actual).read_text(encoding="utf-8"))
        run_id = None
        if isinstance(actual, dict):
            run_id = actual.get("run_id")
            actual = actual.get("samples")
        limits = {key: getattr(args, key) for key in LIMIT_METRICS if getattr(args, key) is not None}
        result = compare_trajectory(reference, actual, limits=limits, max_sample_gap_s=args.max_sample_gap_s)
        if run_id is not None:
            result["run_id"] = run_id
    except (ValueError, OSError, OverflowError) as error:
        print(json.dumps({"data_valid": False, "passed": None, "error": str(error)}))
        return 2
    print(json.dumps(result, indent=2, allow_nan=False))
    return 1 if result["passed"] is False else 0


if __name__ == "__main__":
    raise SystemExit(main())
