import json
import os
print("checker_step_test.py is running")

# Directory of checker_step_test.py
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

# Paths to JSON files
STEP_TEST_PATH = os.path.join(
    BASE_DIR,
    "..", "generateTests", "JSONtests", "step_test.json"
)

DUMMY_TEST_PATH = os.path.join(
    BASE_DIR,
    "dummy_step_test.json"
)

# Load JSON files
with open(STEP_TEST_PATH, "r") as f:
    original_data = json.load(f)

with open(DUMMY_TEST_PATH, "r") as f:
    test_data = json.load(f)

# Index by Message_id
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
        if orig_entry[field] != test_entry[field]:
            mismatches.append(
                f"Message_id {msg_id}, field '{field}': "
                f"original={orig_entry[field]}, test={test_entry[field]}"
            )

# Results
if not mismatches:
    print("✅ All values match!")
else:
    print("❌ Mismatches found:")
    for m in mismatches:
        print(m)
