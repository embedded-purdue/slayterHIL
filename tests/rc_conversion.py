import json
import sys

CONVERSION_FACTOR = 0.5  # 1 RC command = 0.5 meters

# Maps axis delta sign to RC command character
X_POS, X_NEG = "R", "L"
Y_POS, Y_NEG = "F", "B"
Z_POS, Z_NEG = "U", "D"


def interleave_commands(axis_commands):
    """
    Interleave RC commands across axes proportionally using a Bresenham-like
    distribution, so diagonal movement outputs interleaved commands (e.g. RURURU)
    instead of sequential blocks (e.g. RRRRUUU).

    axis_commands: list of (command_char, num_steps) for each active axis
    Returns: list of command characters in interleaved order
    """
    active = [(cmd, steps) for cmd, steps in axis_commands if steps > 0]
    if not active:
        return []

    max_steps = max(steps for _, steps in active)
    errors = [0] * len(active)
    result = []

    for _ in range(max_steps):
        for i, (cmd, steps) in enumerate(active):
            errors[i] += steps
            if errors[i] >= max_steps:
                result.append(cmd)
                errors[i] -= max_steps

    return result


def delta_to_commands(dx, dy, dz):
    """Convert position deltas to an interleaved list of RC command characters."""
    x_steps = int(round(abs(dx) / CONVERSION_FACTOR))
    y_steps = int(round(abs(dy) / CONVERSION_FACTOR))
    z_steps = int(round(abs(dz) / CONVERSION_FACTOR))

    x_cmd = X_POS if dx >= 0 else X_NEG
    y_cmd = Y_POS if dy >= 0 else Y_NEG
    z_cmd = Z_POS if dz >= 0 else Z_NEG

    return interleave_commands([
        (x_cmd, x_steps),
        (y_cmd, y_steps),
        (z_cmd, z_steps),
    ])


def pos_to_rc_commands(data):
    """
    Convert a list of positional waypoint dicts to RC commands.

    Each dict must have at minimum: X_pos, Y_pos, Z_pos
    Returns a list of command characters.
    """
    all_commands = []
    for i in range(1, len(data)):
        prev = data[i - 1]
        curr = data[i]

        dx = curr["X_pos"] - prev["X_pos"]
        dy = curr["Y_pos"] - prev["Y_pos"]
        dz = curr["Z_pos"] - prev["Z_pos"]

        step_commands = delta_to_commands(dx, dy, dz)
        all_commands.extend(step_commands)

    return all_commands


def load_json(path):
    with open(path, "r") as f:
        return json.load(f)


def main():
    if len(sys.argv) < 2:
        print("Usage: python rc_conversion.py <path_to_json>")
        print("       Converts any positional JSON test file to RC commands.")
        sys.exit(1)

    path = sys.argv[1]
    data = load_json(path)
    commands = pos_to_rc_commands(data)

    print("Generated RC Commands:")
    print("".join(commands))
    print(f"\nTotal Commands: {len(commands)}")


if __name__ == "__main__":
    main()
