#!/bin/bash
# This script automates the setup of the VS Code workspace for this project.

# 0. Update west
west update

# Determine the script's location to make it runnable from anywhere
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
APP_DIR=$(dirname "$SCRIPT_DIR")
WORKSPACE_ROOT=$(dirname "$APP_DIR")
APP_DIR_NAME=$(basename "$APP_DIR")

# 1. Copy the vscode configuration directory to the workspace root
VSCODE_SOURCE_PATH="$SCRIPT_DIR/vscode"
VSCODE_DEST_PATH="$WORKSPACE_ROOT/.vscode"
if [ -d "$VSCODE_SOURCE_PATH" ]; then
    echo "Copying vscode directory to workspace root..."
    cp -r "$VSCODE_SOURCE_PATH" "$VSCODE_DEST_PATH"
else
    echo "vscode directory not found in $SCRIPT_DIR. Skipping copy."
fi

# 2. Update the APP_NAME in the justfile
JUSTFILE_PATH="$APP_DIR/justfile"

if [ -f "$JUSTFILE_PATH" ]; then
    echo "Updating APP_NAME in $JUSTFILE_PATH..."
    # Use sed to replace the value of APP_NAME.
    # This handles both macOS and Linux sed versions.
    sed -i.bak "s/\(APP_NAME[[:space:]]*:=[[:space:]]*\).*/\1\""$APP_DIR_NAME"\"/" "$JUSTFILE_PATH" && rm "${JUSTFILE_PATH}.bak"
    echo "APP_NAME updated to '$APP_DIR_NAME'."
else
    echo "justfile not found at $JUSTFILE_PATH. Skipping update."
fi

# 3. Update APP_NAME in c_cpp_properties.json
CPP_PROPERTIES_PATH="$VSCODE_DEST_PATH/c_cpp_properties.json"

if [ -f "$CPP_PROPERTIES_PATH" ]; then
    echo "Updating APP_NAME in $CPP_PROPERTIES_PATH..."
    sed -i.bak "s/{{APP_NAME}}/$APP_DIR_NAME/g" "$CPP_PROPERTIES_PATH" && rm "${CPP_PROPERTIES_PATH}.bak"
    echo "APP_NAME updated in c_cpp_properties.json."
else
    echo "c_cpp_properties.json not found at $CPP_PROPERTIES_PATH. Skipping update."
fi

echo "Setup complete."
