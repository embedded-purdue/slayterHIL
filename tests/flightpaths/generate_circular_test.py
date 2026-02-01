import json
import math

def generate_circular_test():
    data = []
    message_id = 1
    dt = 0.5          # timestamp increment
    timestamp = 0.0

    # Parameters
    Z_max = 7.5       # meters
    radius = 10.0     # circle radius (meters)

    # Phase durations
    ascend_time = 3.0     # seconds (6 points)
    circle_time = 9.0     # seconds (18 points)
    descend_time = 3.0    # seconds (6 points)

    # Velocities
    Z_velocity_up = Z_max / ascend_time
    Z_velocity_down = -Z_max / descend_time
    omega = 2 * math.pi / circle_time   # rad/s

    # -------------------------
    # Phase 1: Ascend (X=R, Y=0)
    # -------------------------
    t = 0.0
    while t < ascend_time:
        entry = {
            "Message_id": message_id,
            "Timestamp": round(timestamp, 2),
            "X_pos": radius,
            "Y_pos": 0,
            "Z_pos": round(Z_velocity_up * t, 2),
            "X_vel_ext": 0,
            "Y_vel_ext": 0,
            "Z_vel_ext": Z_velocity_up
        }
        data.append(entry)
        t += dt
        timestamp += dt
        message_id += 1

    # ---------------------------------
    # Phase 2: Circular motion (constant Z)
    # ---------------------------------
    t = 0.0
    while t < circle_time:
        theta = omega * t
        entry = {
            "Message_id": message_id,
            "Timestamp": round(timestamp, 2),
            "X_pos": round(radius * math.cos(theta), 2),
            "Y_pos": round(radius * math.sin(theta), 2),
            "Z_pos": Z_max,
            "X_vel_ext": round(-radius * omega * math.sin(theta), 2),
            "Y_vel_ext": round( radius * omega * math.cos(theta), 2),
            "Z_vel_ext": 0
        }
        data.append(entry)
        t += dt
        timestamp += dt
        message_id += 1

    # -------------------------
    # Phase 3: Descend (X=R, Y=0)
    # -------------------------
    t = 0.0
    while t < descend_time:
        entry = {
            "Message_id": message_id,
            "Timestamp": round(timestamp, 2),
            "X_pos": radius,
            "Y_pos": 0,
            "Z_pos": round(Z_max + Z_velocity_down * t, 2),
            "X_vel_ext": 0,
            "Y_vel_ext": 0,
            "Z_vel_ext": Z_velocity_down
        }
        data.append(entry)
        t += dt
        timestamp += dt
        message_id += 1

    # Write to JSON file
    with open("circular_test.json", "w") as f:
        json.dump(data, f, indent=4)

    print(f"Generated {len(data)} points → circular_test.json")

# Run the generator
generate_circular_test()
