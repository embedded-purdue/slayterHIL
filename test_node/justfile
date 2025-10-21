# ------------------
#  Configurables
# ------------------
SIM_BOARD        := "qemu_x86"

ESP_BOARD        := "esp32s3_devkitc/esp32s3/procpu"
ESP_PORT         := "/dev/ttyUSB0"          # adjust

CMAKE_CACHE_ARGS := "-- -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"

# set shell := ["bash", "-eu", "-o", "pipefail", "-c"]

# ------------------
#  Helpers
# ------------------
_verify_workspace:
  test -d .west || { echo "ERROR: .west not found here. Ensure in firmware/ and README installation instructions have been followed."; exit 1; }

# ------------------
#  Sim (QEMU)
# ------------------

build-sim: _verify_workspace clean
  west build \
    -p always \
    -b {{SIM_BOARD}} \
    -d build \
    app \
    {{CMAKE_CACHE_ARGS}}

run-sim: build-sim
  west build -t run -d build

# ------------------
#  ESP32-S3
# ------------------

build-esp32: _verify_workspace clean
  west build \
    -p always \
    -b "{{ESP_BOARD}}" \
    -d build \
    app \
    {{CMAKE_CACHE_ARGS}} \
    -DEXTRA_DTC_OVERLAY_FILE="{{justfile_directory()}}/app/boards/esp32s3_devkitc.overlay"

build-esp32-spi-example: _verify_workspace clean
  west build \
    -p always \
    -b "{{ESP_BOARD}}" \
    -d build \
    spi_example_app \
    {{CMAKE_CACHE_ARGS}} \
    -DEXTRA_DTC_OVERLAY_FILE="{{justfile_directory()}}/spi_example_app/boards/spi_esp32s3_devkitc.overlay"

flash-esp32:
  west flash --build-dir build

monitor-esp32:
  python -m serial.tools.miniterm "{{ESP_PORT}}" 115200

run-esp32: build-esp32 flash-esp32 monitor-esp32

run-esp32-spi-example: build-esp32-spi-example flash-esp32 monitor-esp32

attach-wsl-usb-port:
  #!/usr/bin/env bash
  set -euo pipefail

  BUSID=$(usbipd.exe list | grep "CP2102N USB to UART Bridge Controller" | awk 'NR==1 {print $1}')

  if [ -z "$BUSID" ]; then
    echo "Device not found: CP2102N USB to UART Bridge Controller"
    echo "This error may be because you are using a USB adapter."
    exit 1
  fi

  echo "Found device on BUSID: $BUSID"

  usbipd.exe bind --busid "$BUSID"
  usbipd.exe attach --wsl --busid "$BUSID"

  echo "Device attached to WSL."


# ------------------
#  Defaults
# ------------------

clean:
  rm -rf build
