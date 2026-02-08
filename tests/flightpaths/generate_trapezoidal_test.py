import json

def generate_trapezoidal_test():
    data = []
    message_id = 1
    dt = 0.5
    timestamp = 0.0

    # Velocities
    Z_velocity = 75 / 5  # m/s
    X_velocity = 75 / 5   # m/s

    # Phase 1: Ascend + move forward (0-5 s)
    t = 0.0
    while t < 5.0:
        entry = {
            "Message_id": message_id,
            "Timestamp": round(timestamp, 2),
            "X_pos": round(X_velocity * t, 2),
            "Y_pos": 0,
            "Z_pos": round(Z_velocity * t, 2),
            "X_vel_ext": X_velocity,
            "Y_vel_ext": 0,
            "Z_vel_ext": Z_velocity
        }
        data.append(entry)
        t += dt
        timestamp += dt
        message_id += 1

    # Phase 2: Cruise at max height (5-10 s)
    t = 0.0
    while t < 5.0:
        entry = {
            "Message_id": message_id,
            "Timestamp": round(timestamp, 2),
            "X_pos": round(75 + X_velocity * t, 2),
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

    # Phase 3: Descend + move forward (10-15 s)
    t = 0.0
    while t <= 5.0:  # include endpoint
        entry = {
            "Message_id": message_id,
            "Timestamp": round(timestamp, 2),
            "X_pos": round(150 + Z_velocity * t, 2),
            "Y_pos": 0,
            "Z_pos": round(75 - Z_velocity * t, 2),
            "X_vel_ext": Z_velocity,
            "Y_vel_ext": 0,
            "Z_vel_ext": -Z_velocity
        }
        data.append(entry)
        t += dt
        timestamp += dt
        message_id += 1

    # Save JSON
    with open("trapezoidal_test.json", "w") as f:
        json.dump(data, f, indent=4)

    print(f"Generated {len(data)} points → trapezoidal_test.json")

# Run generator
generate_trapezoidal_test()
