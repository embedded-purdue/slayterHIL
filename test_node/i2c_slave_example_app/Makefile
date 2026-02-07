PORT ?= /dev/ttyUSB0
BOARD = esp32s3_devkitc/esp32s3/procpu
ARGS = -- -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
WD = $(shell pwd)
BUILD = ../build
VENV = ../../.venv/bin/activate

monitor:
	source $(VENV) && python -m serial.tools.miniterm $(PORT) 115200

target: build-target flash-target
	echo "Building and flashing target complete."

controller: build-controller flash-controller
	echo "Building and flashing controller complete."

flash-target: build-target
	source $(VENV) && west flash --build-dir=$(BUILD) --esp-device=$(PORT)

flash-controller: build-controller
	source $(VENV) && west flash --build-dir=$(BUILD) --esp-device=$(PORT)

build-controller: clean
	source $(VENV) && west build -p always \
	  -b $(BOARD) \
	  -d $(BUILD) \
	  controller \
	  $(ARGS) \
	  -DEXTRA_DTC_OVERLAY_FILE="$(WD)/controller/boards/esp32s3_devkitc.overlay"

build-target: clean
	source $(VENV) && west build -p always \
	  -b $(BOARD) \
	  -d $(BUILD) \
	  custom_target \
	  $(ARGS) \
	  -DEXTRA_DTC_OVERLAY_FILE="$(WD)/custom_target/boards/esp32s3_devkitc.overlay"

clean:
	echo "cleaning old build"
	rm -rf ../build

.PHONY: flash-target flash-controller build-controller build-target clean venv target controller