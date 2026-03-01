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

        changes = 0
        if dx != 0: 
            changes = changes + 1
        if dy != 0:
            changes = changes + 1
        if dz != 0:
            changes = changes + 1
        
        if changes < 2:
        
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

        
        if changes == 2:
            xsteps = int(round(abs(dx) / CONVERSION_FACTOR))
            ysteps = int(round(abs(dy) / CONVERSION_FACTOR))
            zsteps = int(round(abs(dz) / CONVERSION_FACTOR))
            # find the gcf of the two steps that changed
            # then divide by gcf to see ratio
            if dx != 0 and dz != 0:
                factor = math.gcd(xsteps, zsteps)
                r1 = int(xsteps / factor)
                r2 = int(zsteps / factor)
                total1steps = 0
                total2steps = 0
                while total1steps < xsteps or total2steps < zsteps:
                    for i in range(0, r1):
                        if dx > 0:
                            commands.extend(["R"])
                        else: 
                            commands.extend(["L"])
                        total1steps = total1steps + 1
                    for i in range(0, r2):
                        if dz > 0:
                            commands.extend(["U"])
                        else: 
                            commands.extend(["D"])
                        total2steps = total2steps + 1
            if dx != 0 and dy != 0:
                factor = math.gcd(xsteps, ysteps)
                r1 = int(xsteps / factor)
                r2 = int(ysteps / factor)
                total1steps = 0
                total2steps = 0
                while total1steps < xsteps or total2steps < ysteps:
                    for i in range(0, r1):
                        if dx > 0:
                            commands.extend(["R"])
                        else:
                            commands.extend(["L"])
                        total1steps = total1steps + 1
                    for i in range(0, r2):
                        if dy > 0:
                            commands.extend(["F"])
                        else:
                            commands.extend(["B"])
                        total2steps = total2steps + 1
            if dy != 0 and dz != 0:
                factor = math.gcd(ysteps, zsteps)
                r1 = int(ysteps / factor)
                r2 = int(zsteps / factor)
                total1steps = 0
                total2steps = 0
                while total1steps < ysteps or total2steps < zsteps:
                    for i in range(0, r1):
                        if dy > 0:
                            commands.extend(["F"])
                        else:
                            commands.extend(["B"])
                        total1steps = total1steps + 1
                    for i in range(0, r2):
                        if dz > 0:
                            commands.extend(["U"])
                        else:
                            commands.extend(["D"])
                        total2steps = total2steps + 1

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