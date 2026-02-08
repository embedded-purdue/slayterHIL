import json

with open("step_test.json", "r") as f:
    original_data = json.load(f)

with open("dummy_step_test.json", "r") as f:
    test_data = json.load(f)

original_by_id = {entry["Message_id"]: entry for entry in original_data}
test_by_id = {entry["Message_id"]: entry for entry in test_data}

fields_to_compare = [
    "Timestamp",
    "X_pos", "Y_pos", "Z_pos",
    "X_vel_ext", "Y_vel_ext", "Z_vel_ext"
]

mismatches = []

for msg_id, orig_entry in original_by_id.items():
    if msg_id not in test_by_id:
        mismatches.append(f"Message_id {msg_id} missing in test file")
        continue

    test_entry = test_by_id[msg_id]

    for field in fields_to_compare:
        orig_val = orig_entry[field]
        test_val = test_entry[field]

        if orig_val != test_val:
            mismatches.append(
                f"Message_id {msg_id}, field '{field}': "
                f"original={orig_val}, test={test_val}"
            )
if not mismatches:
    print("✅ All values match!")
else:
    print("❌ Mismatches found:")
    for m in mismatches:
        print(m)
