# Flight Simulator

Flight simulation branch.

## Default simulation build

From `flight_sim`, build and run the simulation in `src/old_main.cpp`:

```sh
bash setup.sh
cd build
./flight_sim --capture spi_frames.bin
```

The `--capture` option runs the physics simulation without a Pi or STM32.
It writes `spi_frames.bin` and `old_main_pid_tuning.csv` in the working
directory. For a short run, add `--samples 120`. Without `--capture`,
the old-main entry point attempts Linux SPI transmission.

The Pi sender is excluded from the default build. Build it explicitly with
`cmake --build build --target old_main_spi_test` when needed.
`setup.sh` accepts additional CMake configuration arguments; see the Intel
Protobuf instructions below for this Mac's dependencies.

## Fall 2025 Contributions

**Data Packaging and SPI Communication**

    - `spi_new_test.cpp`: Creates test data, packages it into a protobuf header, and performs a SPI transfer from the RPI to the ESP32
            - This is the outline for future data transfer, but needs to be updated so the protobuf data is set to the generated output from the physics engine. `
    - `../shared/sensor_data.proto` defines the fields to package sensor data for transfer to the testnode
    - `../../testnode/.../spi_slave_protobuf.c`: Code for the esp32 side of the SPI communication

**Flight sim**

    - `spi_new_test.cpp`: Creates test data, packages it into a protobuf header, and performs a SPI transfer from the RPI to the ESP32
            - This is the outline for future data transfer, but needs to be updated so the protobuf data is set to the generated output from the physics engine. `
    
    - `../shared/sensor_data.proto` defines the fields to package sensor data for transfer to the testnode
    
    - `../../testnode/.../spi_slave_protobuf.c`: Code for the esp32 side of the SPI communication


## IMU wire frame capture

From `flight_sim`:

```sh
cmake -S . -B build
cmake --build build -j 4
ctest --test-dir build --output-on-failure
cd build
./flight_sim
```

Run from the build directory so the relative flightpath input resolves.
`main.cpp` encodes one IMU sample per simulation tick and writes it to
`spi_frames.bin` (replaced each run). Each record is exactly 256 bytes:

```
[uint16 little-endian protobuf length][SensorPacket payload][zero padding]
```

The payload uses `shared/proto/sensor_data.proto`, matching the active test-node
scheduler. Signed LSB/MSB IMU pairs are reconstructed without rescaling.
The encoder accepts a timestamp in microseconds, LiDAR distance in millimetres,
and RC axes. The capture currently supplies simulation-relative timestamps,
LiDAR = 0 mm, and RC = 0 (no-op).

This captures transport-ready frames; it does not open an SPI device. Live
transmission must send each entire frame in one transaction and map timestamps
to the receiver clock (or use an explicit immediate-injection receiver mode).
LiDAR zero is an actual distance update in the combined receiver packet.
IMU physics/unit corrections beyond initializing its state remain separate work.

On a Mac with only Intel Protobuf installed, use a matching Intel build:

```sh
cmake -S . -B build-intel -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_PREFIX_PATH=/usr/local -Dabsl_DIR=/usr/local/opt/abseil/lib/cmake/absl
cmake --build build-intel -j 4
ctest --test-dir build-intel --output-on-failure
cd build-intel
./flight_sim
```

This requires macOS support for running Intel executables. Native ARM builds
require ARM Protobuf and its dependencies.

## Pi → STM32 test using the original main loop

`old_main_spi_test` builds `src/old_main.cpp`, recovered from the original
committed `main.cpp`. It retains the waypoint path, position/velocity controllers,
and 1/60-second physics step. It uses the current rigid-body and IMU code, so
its values are not bit-for-bit reproductions of the old physics bugs.

On the Pi, from `flight_sim` (with Eigen, Protobuf and CMake installed):

```sh
cmake -S . -B build
cmake --build build --target old_main_spi_test -j 4
./build/old_main_spi_test --device /dev/spidev0.0 --samples 1800
```

Enable SPI on the Pi and ensure your user can open `/dev/spidev0.0`.
The test opens SPI once, uses mode 0 / 8 bits / MSB first / 500 kHz, and sends
one complete 256-byte frame per sample. Its nominal 60 Hz simulation uses a
full wait between sends, so wall-clock delivery is slightly slower than 60 Hz.
`--samples 120` gives a short bring-up run. `--path FILE` selects a flightpath.
The default input resolves from the repository independently of the working directory.

Each frame contains the simulated IMU, LiDAR placeholder 1000 mm, zero RC,
and explicit `IMMEDIATE` injection. The timestamp is simulation time, not an
STM32 clock deadline. Rebuild and flash the updated test-node firmware before
using this sender; older firmware will not implement this timing mode.

For the checked-in **Nucleo H563ZI SPI4 overlay**, connect:

| Pi signal | STM32 signal |
| --- | --- |
| SCLK | PE2 (SPI4 SCK) |
| MOSI | PE6 (SPI4 MOSI) |
| CE0 | PE4 (SPI4 NSS) |
| MISO | PE5 (SPI4 MISO; receiver is currently receive-only) |
| Ground | Ground |

Use 3.3 V logic and confirm the header locations for your board. Start the
STM32 firmware and serial monitor before starting the sender. In an initialized
Zephyr/west workspace, build the firmware from `test_node` with:

```sh
west build -p always -b nucleo_h563zi -d build app
west flash -d build
```

The board overlay in `app/boards/nucleo_h563zi.overlay` is selected by the board
name. Confirm the generated devicetree uses SPI4 and the pins above.

Compare the Pi's `TX #... sim_us=... euler=... gyro=... acc=...` lines with
STM32 `RX` lines **by timestamp**. The receiver's `rx` and `applied` counters
should rise, and `decode_fail` should remain zero. SPI completion on the Pi
only means bytes were clocked out; it is not a receiver acknowledgment.
No hardware link has been verified from the development Mac.

Desktop verification uses the same loop and encoder with file output:

```sh
./build-intel/old_main_spi_test --capture old_main_frames.bin --samples 120
```

Each binary record is exactly 256 bytes. The test also writes
`old_main_pid_tuning.csv` in the working directory. Capture mode replaces the
output file and never opens an SPI device. The hardware test is deliberately
not registered as an automatic CTest test.
