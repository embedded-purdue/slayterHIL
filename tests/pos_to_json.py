"""
pos_to_json.py

Generic converter: takes any incoming positional data and outputs a
standard SlayterHIL JSON package.

Accepted input formats
----------------------
1. A JSON file whose records have at minimum X_pos, Y_pos, Z_pos, and
   optionally Timestamp, Message_id, X_vel_ext, Y_vel_ext, Z_vel_ext.
2. A CSV file with columns: timestamp, x, y, z  (and optionally vx, vy, vz).

Missing optional fields are filled with sensible defaults:
  - Timestamp    : auto-incremented by DT (0.5 s)
  - Message_id   : auto-incremented from 1
  - *_vel_ext    : estimated from consecutive position deltas / DT
  - Direction    : derived from velocity vectors

Usage
-----
    python pos_to_json.py <input_file> [output_file]

    Input  : JSON or CSV
    Output : JSON file in the standard SlayterHIL format (default: out.json)
"""

import json
import os
import sys

DT = 0.5  # default time step (seconds)


# ---------------------------------------------------------------------------
# Direction helper
# ---------------------------------------------------------------------------

def compute_direction(x_vel, y_vel, z_vel):
    dirs = []
    if x_vel > 0:
        dirs.append("R")
    elif x_vel < 0:
        dirs.append("L")
    if y_vel > 0:
        dirs.append("F")
    elif y_vel < 0:
        dirs.append("B")
    if z_vel > 0:
        dirs.append("U")
    elif z_vel < 0:
        dirs.append("D")
    return "".join(dirs) if dirs else "HOVER"


# ---------------------------------------------------------------------------
# Loaders
# ---------------------------------------------------------------------------

def load_json_input(path):
    with open(path, "r") as f:
        raw = json.load(f)
    if not isinstance(raw, list):
        raise ValueError("JSON input must be a list of position records.")
    return raw


def load_csv_input(path):
    import csv
    records = []
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            records.append({k.strip(): v.strip() for k, v in row.items()})
    return records


# ---------------------------------------------------------------------------
# Normalisation
# ---------------------------------------------------------------------------

_X_ALIASES = ("X_pos", "x_pos", "x", "X")
_Y_ALIASES = ("Y_pos", "y_pos", "y", "Y")
_Z_ALIASES = ("Z_pos", "z_pos", "z", "Z")
_T_ALIASES = ("Timestamp", "timestamp", "t", "time")
_ID_ALIASES = ("Message_id", "message_id", "id", "ID")
_VX_ALIASES = ("X_vel_ext", "x_vel_ext", "vx", "VX")
_VY_ALIASES = ("Y_vel_ext", "y_vel_ext", "vy", "VY")
_VZ_ALIASES = ("Z_vel_ext", "z_vel_ext", "vz", "VZ")


def _get(record, aliases, default=None):
    for key in aliases:
        if key in record:
            return record[key]
    return default


def normalise_records(raw_records):
    """
    Accept records with any supported key names and return a list of dicts
    with only the canonical SlayterHIL keys, estimating missing values.
    """
    records = []
    for i, r in enumerate(raw_records):
        x = float(_get(r, _X_ALIASES, 0))
        y = float(_get(r, _Y_ALIASES, 0))
        z = float(_get(r, _Z_ALIASES, 0))
        t = _get(r, _T_ALIASES)
        mid = _get(r, _ID_ALIASES)
        vx = _get(r, _VX_ALIASES)
        vy = _get(r, _VY_ALIASES)
        vz = _get(r, _VZ_ALIASES)

        records.append({
            "_idx": i,
            "X_pos": x,
            "Y_pos": y,
            "Z_pos": z,
            "Timestamp": float(t) if t is not None else round(i * DT, 2),
            "Message_id": int(mid) if mid is not None else i + 1,
            "X_vel_ext": float(vx) if vx is not None else None,
            "Y_vel_ext": float(vy) if vy is not None else None,
            "Z_vel_ext": float(vz) if vz is not None else None,
        })

    # Fill in missing velocity estimates using finite differences
    for i, rec in enumerate(records):
        if rec["X_vel_ext"] is None:
            if i + 1 < len(records):
                dt = records[i + 1]["Timestamp"] - rec["Timestamp"] or DT
                rec["X_vel_ext"] = round((records[i + 1]["X_pos"] - rec["X_pos"]) / dt, 4)
            else:
                rec["X_vel_ext"] = 0.0
        if rec["Y_vel_ext"] is None:
            if i + 1 < len(records):
                dt = records[i + 1]["Timestamp"] - rec["Timestamp"] or DT
                rec["Y_vel_ext"] = round((records[i + 1]["Y_pos"] - rec["Y_pos"]) / dt, 4)
            else:
                rec["Y_vel_ext"] = 0.0
        if rec["Z_vel_ext"] is None:
            if i + 1 < len(records):
                dt = records[i + 1]["Timestamp"] - rec["Timestamp"] or DT
                rec["Z_vel_ext"] = round((records[i + 1]["Z_pos"] - rec["Z_pos"]) / dt, 4)
            else:
                rec["Z_vel_ext"] = 0.0

    return records


# ---------------------------------------------------------------------------
# Package builder
# ---------------------------------------------------------------------------

def build_package(records):
    package = []
    for rec in records:
        entry = {
            "Message_id": rec["Message_id"],
            "Timestamp": rec["Timestamp"],
            "X_pos": rec["X_pos"],
            "Y_pos": rec["Y_pos"],
            "Z_pos": rec["Z_pos"],
            "X_vel_ext": rec["X_vel_ext"],
            "Y_vel_ext": rec["Y_vel_ext"],
            "Z_vel_ext": rec["Z_vel_ext"],
            "Direction": compute_direction(
                rec["X_vel_ext"], rec["Y_vel_ext"], rec["Z_vel_ext"]
            ),
        }
        package.append(entry)
    return package


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def convert(input_path, output_path="out.json"):
    ext = os.path.splitext(input_path)[1].lower()
    if ext == ".json":
        raw = load_json_input(input_path)
    elif ext == ".csv":
        raw = load_csv_input(input_path)
    else:
        raise ValueError(f"Unsupported file type: {ext}. Use .json or .csv")

    records = normalise_records(raw)
    package = build_package(records)

    with open(output_path, "w") as f:
        json.dump(package, f, indent=4)

    print(f"Converted {len(package)} records → {output_path}")
    return package


def main():
    if len(sys.argv) < 2:
        print("Usage: python pos_to_json.py <input_file> [output_file]")
        print("  input_file  : .json or .csv with positional data")
        print("  output_file : output path (default: out.json)")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2] if len(sys.argv) > 2 else "out.json"
    convert(input_path, output_path)


if __name__ == "__main__":
    main()
