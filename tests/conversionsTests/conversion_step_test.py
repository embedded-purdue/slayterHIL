import json
import os
import math

CONVERSION_FACTOR = 0.5  # 1 command = 0.5 meters

def load_json(path):
    with open(path, "r") as f:
        return json.load(f)


def convert_to_commands(data):
    commands = []

    for i in range(1, len(data)):
        prev = data[i - 1]
        curr = data[i]

        dx = curr["X_pos"] - prev["X_pos"]
        dy = curr["Y_pos"] - prev["Y_pos"]
        dz = curr["Z_pos"] - prev["Z_pos"]

        # X direction
        if dx != 0:
            steps = int(round(abs(dx) / CONVERSION_FACTOR))
            if dx > 0:
                commands.extend(["R"] * steps)
            else:
                commands.extend(["L"] * steps)

        # Y direction
        if dy != 0:
            steps = int(round(abs(dy) / CONVERSION_FACTOR))
            if dy > 0:
                commands.extend(["F"] * steps)
            else:
                commands.extend(["B"] * steps)

        # Z direction
        if dz != 0:
            steps = int(round(abs(dz) / CONVERSION_FACTOR))
            if dz > 0:
                commands.extend(["U"] * steps)
            else:
                commands.extend(["D"] * steps)

    return commands


def main():
    json_path = "step_test.json"

    data = load_json(json_path)
    commands = convert_to_commands(data)

    print("Generated Commands:")
    print("".join(commands))
    print(f"\nTotal Commands: {len(commands)}")


if __name__ == "__main__":
    main()
