import json

def generate_hover_test():
    data = []
    message_id = 1
    dt = 0.5
    timestamp = 0.0

    # Parameters
    phase_time = 5.0   # seconds per phase
    Z_max = 75        # meters

    # Velocities (linear)
    Z_velocity_up = Z_max / phase_time      # +1.5 m/s
    Z_velocity_down = -Z_max / phase_time   # -1.5 m/s

    # -------------------------
    # Phase 1: Ascend (0-5s)
    # -------------------------
    t = 0.0
    while t < phase_time:
        entry = {
            "Message_id": message_id,
            "Timestamp": round(timestamp, 2),
            "X_pos": 0,
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

    # -------------------------
    # Phase 2: Hover (5-10s)
    # -------------------------
    t = 0.0
    while t < phase_time:
        entry = {
            "Message_id": message_id,
            "Timestamp": round(timestamp, 2),
            "X_pos": 0,
            "Y_pos": 0,
            "Z_pos": Z_max,
            "X_vel_ext": 0,
            "Y_vel_ext": 0,
            "Z_vel_ext": 0
        }
        data.append(entry)
        t += dt
        timestamp += dt
        message_id += 1

    # -------------------------
    # Phase 3: Descend (10-15s)
    # -------------------------
    t = 0.0
    while t < phase_time:
        entry = {
            "Message_id": message_id,
            "Timestamp": round(timestamp, 2),
            "X_pos": 0,
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

    # Write JSON
    with open("hover_test.json", "w") as f:
        json.dump(data, f, indent=4)

    print(f"Generated {len(data)} points → hover_test.json")

# Run the generator
generate_hover_test()
