import json

def generate_step_test():
    data = []
    message_id = 1
    dt = 0.5  # timestamp increment
    timestamp = 0.0

    # Phase durations
    phase_time = 5.0  # seconds
    # Velocities
    Z_velocity = 75 / 5  # m/s
    X_velocity = 75 / 5   # m/s

    # Phase 1: Ascend (Z increases, X=0)
    t = 0.0
    while t < phase_time:
        entry = {
            "Message_id": message_id,
            "Timestamp": round(timestamp, 2),
            "X_pos": 0,
            "Y_pos": 0,
            "Z_pos": round(Z_velocity * t, 2),
            "X_vel_ext": 0,
            "Y_vel_ext": 0,
            "Z_vel_ext": Z_velocity
        }
        data.append(entry)
        t += dt
        timestamp += dt
        message_id += 1

    # Phase 2: Move forward (X increases, Z=75)
    t = 0.0
    while t < phase_time:
        entry = {
            "Message_id": message_id,
            "Timestamp": round(timestamp, 2),
            "X_pos": round(X_velocity * t, 2),
            "Y_pos": 0,
            "Z_pos": 75,
            "X_vel_ext": X_velocity,
            "Y_vel_ext": 0,
            "Z_vel_ext": 0
        }
        data.append(entry)
        t += dt
        timestamp += dt
        message_id += 1

    # Phase 3: Descend (Z decreases, X=75)
    t = 0.0
    while t <= phase_time:  # include endpoint to reach Z=0
        entry = {
            "Message_id": message_id,
            "Timestamp": round(timestamp, 2),
            "X_pos": 75,
            "Y_pos": 0,
            "Z_pos": round(75 - Z_velocity * t, 2),
            "X_vel_ext": 0,
            "Y_vel_ext": 0,
            "Z_vel_ext": -Z_velocity
        }
        data.append(entry)
        t += dt
        timestamp += dt
        message_id += 1

    # Write to JSON file
    with open(r"JSONTests\step_test.json", "w") as f:
        json.dump(data, f, indent=4)

    print(f"Generated {len(data)} points → step_test.json")

# Run the generator
generate_step_test()
