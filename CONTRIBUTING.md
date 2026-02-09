# Contributor Documentation

The following is documentation related to information for every SlayterHIL subteam. Technical information regarding to contributing to a particular part of the codebase is outlined, as well as a member registry and branch usage per subteam.

---

# Table of Contents

- [Flight Simulation](#flight-simulation)
  - [Development and Branching](#development-and-branching)
  - [Team members](#team-members)
  - [SSH Connection for RPI](#ssh-connection-for-rpi)
  - [Technical Documentation](#technical-documentation)
- [Test Node](#test-node)
  - [Test node setup](#test-node-setup)

---

# Flight Simulation

*Last Updated: February 9, 2026*

  - [Development and Branching](#development-and-branching)
    - [Development methodology](#development-methodology)
    - [Currently active git submodules](#currently-active-git-submodules)
    - [Currently active branches](#currently-active-branches)
    - [Branch backups](#branch-backups)
  - [Team members](#team-members)
    - [Team leader](#team-leader)
    - [Active members](#active-members)
    - [All members](#all-members)
  - [SSH Connection for RPI](#ssh-connection-for-rpi)
  - [Technical Documentation](#technical-documentation)
    - [Contribution guidelines](#contribution-guidelines)
    - [Connection from Test Automation](#connection-from-test-automation)
    - [General CMake file](#general-cmake-file)


## Development and Branching

### Development methodology

At the current moment, flight simulation is being developed through git submodules placed inside the root of the `SlayterHil` repository. To clone the appropriate submodules, make sure to run the following cloning command.

```text
git clone --recurse-submodules https://github.com/embedded-purdue/slayterHIL.git
```

Then check out the main branch inside the module used to access the flight simulation repository.

### Currently active git submodules

A lot of code for flight simulation is being developed in parallel inside of the ```flight_sim_repo``` submodule, which can be accessed through the submodule [link](https://github.com/embedded-purdue/slayterHIL-flight-sim).

### Currently active branches

Flightsim/FS/flight_sim.

### Branch backups

physics_sim_environment_setup_bak, simulator_PIDcontrol_bak, spi-comp-bak, spi-comp-bak-bak, flight-graphics-bak.

## Team members

### Team leader

Murtaza R. Haque

### Active members

Murtaza R. Haque, Frank Lee Schlehofer, Sebastian Ting, Ramazan Kaan Cetin, Benjamin Lobos Lertpunyaroj, Maanav Jugal Shah, Gautam Aravindan

### All members

Murtaza R. Haque, Frank Lee Schlehofer, Sebastian Ting, Ramazan Kaan Cetin, Benjamin Lobos Lertpunyaroj, Maanav Jugal Shah, Gautam Aravindan, Matt Stein

Flight Sim will never forget Alex Aylward 🐐


## SSH Connection for RPI

*Last Updated: February 8, 2026.*

Connections to RPI are handled through Tailscale. It is a mesh VPN that makes it seem like all devices connected are in the same private network, allowing anybody to ssh into the remote device. Appropriate downloads are found through the website links.

To add your device to the Tailscale network, you can use the SlayterHil Google account and password that are seen inside of the Flight Sim Documentation file (found in the organizations Google Drive) to sign in inside the Tailscale website.

The RPI, which is named `raspi-5-orcheslayter` at the time of writing, can be accessed through the `flight_sim` user. Information to SSH can be seen inside of the tailscale account or the Flight Sim Documentation file.

## Technical Documentation

### Contribution guidelines

For contributing documentation, please specify your name in the Author(s) field and also specify the time last updated below the correct section to keep track of changes.

Additionally, consider writing in Markdown format to allow for later compatibility with website documentation migration in the future.

### Connection from Test Automation

*Authors: Benjamin Lobos Lertpunyaroj & Frankie Schlehofer.*
*Last Updated: February 8, 2026.*

Testing is received from the Test Automation team through a JSON file specified at a directory of their choosing. More up-to-date information regarding the formatting of the data sent can be seen in their internal design documentation.

At this time, most of the connection is handled natively inside the RPI, and parsing of this data is being handled in a development branch inside the flight simulation submodule, `/utils/json_utils.cpp`.

The library used to parse JSON in C++ is `nlohmann/json`, which works through the C++ header file, `/utils/json.hpp`, and setup has been handled inside the local flight simulation CMake file `/CMakeLists.txt`.

### General CMake file

*Authors: Benjamin Lobos Lertpunyaroj & Frankie Schlehofer.*
*Last Updated: February 8, 2026.*

This CMake configuration requires `nlohmann_json` and links it through the imported target `nlohmann_json::nlohmann_json`.

Protobuf generation is configured from the shared proto directory `../shared/proto` by globbing all `*.proto` files. Generated C++ outputs (`*.pb.cc` / `*.pb.h`) are written to `${CMAKE_CURRENT_BINARY_DIR}/generated`, which corresponds to the build output directory (e.g., `flight_sim_repo/build/generated/`).

A static library `proto_lib` is created to compile the generated Protobuf sources. `protobuf_generate(...)` attaches generated sources to `proto_lib`, uses `${CMAKE_SOURCE_DIR}/../` as an import root for resolving proto imports, and outputs generated files into `${GEN_PROTO_DIR}`. The configured source list for `proto_lib` is printed during CMake configure time using `get_target_property(... SOURCES)` and `message(STATUS ...)`.

`proto_lib` links publicly to `protobuf::libprotobuf` and exports include directories `${GEN_PROTO_DIR}` (generated headers) and `${PROTO_DIR}` (proto definitions) as `PUBLIC` include paths so downstream targets inherit them automatically.

The following executables are defined: `json_test` (from `utils/json_utils.cpp`), `${PROJECT_NAME}` (from `src/main.cpp` and `src/physics/rigid_body.cpp`), `UDPServer` (from `src/UDPServer.cpp`), and `UDPClient` (from `src/UDPClient.cpp`).

Linking: `json_test` links to `nlohmann_json::nlohmann_json`; `${PROJECT_NAME}` links to `proto_lib`, `Eigen3::Eigen`, and `nlohmann_json::nlohmann_json`; `UDPServer` and `UDPClient` each link to `proto_lib` and `nlohmann_json::nlohmann_json`.

---

# Test Node

## Test node setup

_Note: If you are on a Windows PC, use WSL2 (look up install instructions, ubuntu recommended) and follow the instructions for linux_

1. Install prerequisites

    - [Python3](https://www.geeksforgeeks.org/python/download-and-install-python-3-latest-version/)

    - Complete **only** the [Install Dependencies](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies) section of the [Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#getting-started-guide).

    - [Just command runner](https://github.com/casey/just?tab=readme-ov-file#installation) - This is used like a makefile as a hotkey for all of our project-specific commands
        - If you are using Windows, installing chocolatey package manager is recommended. After installing chocolatey, you can install just by running `choco install just`.
        - If you are on Mac, use the [Homebrew](https://brew.sh/) command `brew install just`.

    - Windows + WSL2: [usbipd-win](https://learn.microsoft.com/en-us/windows/wsl/connect-usb) - This is used to expose the USB ports to WSL2 so that flashing works properly

    - [Protobuf compiler](https://protobuf.dev/installation/)
        - If using WSL/Ubuntu: `sudo apt install -y protobuf-compiler`
        - If using mac: `brew install protobuf`

2. Clone this repository:
    ```bash
    git clone git@github.com:embedded-purdue/slayterHIL.git
    cd slayterHIL
    ```

3. Create and activate a Python virtual environment:
    ```bash
    python -m venv .venv

    source .venv/bin/activate     # Linux/macOS
    .venv\Scripts\activate.bat    # PC, Command Prompt
    .venv\Scripts\Activate.ps1    # PC, Powershell
    source .venv/Scripts/activate # PC. Git bash
    ```

4. Install west
    ```bash
    pip install west
    ```

5. Update the workspace
    ```bash
    cd test_node
    west update
    west packages pip --install
    ```

6. Install the Zephyr SDK
    1. navigate to `slayterHIL/test_node/zephyr`
        ```bash
        cd zephyr
        ```
    2.  Perform installation
        ```bash
        west sdk install
        ```

7. Try to build and run
    1. navigate to `slayterHIL/test_node`
        ```bash
        cd ..
        ```
    2.  Build the sim to verify it works
        ```bash
        just run-sim
        ```

8. Test flash
    1. navigate to `slayterHIL/test_node/zephyr`
    3. (FOR WSL USERS; SKIP IF NOT) plug in board and use usbipd to connect it

       0. as a shortcut, try using `just attach-wsl-usb-port`. This should work as long as your laptop is directly plugged into the UART port on the esp32. **NOTE**: you will still need to add yourself to either the `dialout` or `uucp` user group, as explained below
       1. on Powershell (run with admin), run `usbipd list` and find the busid for the MCU

          *example output:*
          ```bash
          Connected:
          BUSID  VID:PID    DEVICE                                                                      STATE
          1-2    10c4:ea60  CP2102N USB to UART Bridge Controller                         Not shared
          1-3    06cb:016c  Synaptics UWP WBDI                                            Not shared
          1-10   8087:0033  Intel(R) Wireless Bluetooth(R)                                Not shared

          Persisted:
          GUID                                  DEVICE
          ```
       2. run `usbipd bind --busid [busid]` for the example above, [busid] is `1-2`. If it works, there should be no output
       3. run `usbipd attach --wsl --busid [busid]`. This will attach the port to WSL.

          *example output:*
          ```bash
          usbipd: info: Using WSL distribution 'archlinux' to attach; the device will be available in all WSL 2 distributions.
          usbipd: info: Loading vhci_hcd module.
          usbipd: info: Detected networking mode 'nat'.
          usbipd: info: Using IP address 172.21.112.1 to reach the host.
          ```
          *note: make sure the distro is the one you are using (if you have more than one installed)*
       4. now, in your WSL terminal, run `lsusb` and make sure the USB device shows
           1. if `lsusb` is not installed, you may need to install `usbutils` (look up)
    5. (FOR MAC USERS; SKIP IF NOT) plug in board and run:
       1. ```bash
          ls /dev/ | grep tty
          ```
       2. You should see a list of connected devices. We are looking for something similar to either of the following:
          ```bash
          tty.usbserial-210
          tty.usbmodem2101
          ```
          usbserial will appear if you're connected to the expressif's UART path, while usbmodem will appear if you're connected to its USB path
       3. Copy the device name (eg. `tty.usbserial-210`)
       4. Navigate to testnode/justfile and replace the value in the `ESP_PORT` line with your device name. It will now be able to communicate to the board using this name.
          ```
          ESP_PORT         := "/dev/tty.usbserial-210"
          ```
    6. now that the USB is connected, test build the `hello_world` sample by running `west build -p always -b esp32s3_devkitc/esp32s3/procpu samples/hello_world`
    7. flash to the usb with `west flash`
       1. if this is the first time attempting to flash after connecting the USB, you may see an error like the following:

          ```bash
          /dev/ttyUSB0 failed to connect: Could not open /dev/ttyUSB0, the port is busy or doesn't exist.
          ([Errno 13] could not open port /dev/ttyUSB0: [Errno 13] Permission denied: '/dev/ttyUSB0')

          Hint: Try to add user into dialout or uucp group.


          A fatal error occurred: Could not connect to an Espressif device on any of the 1 available serial ports.
          FATAL ERROR: command exited with status 2: esptool --baud 921600 --before default-reset --after hard-reset write-flash -u --flash-mode dio --flash-freq 80m --flash-size 8MB 0x0 /home/evinl/es@p/slayterHIL/test_node/zephyr/build/zephyr/zephyr.bin
          ```
       2. to fix this issue, you need add yourself to the correct usergroup:
          1. run `ls -l /dev/[ttyUSB0]` <- ttyUSB0 is whichever port it complained about in the error

             *example output:*
             ```bash
             crw-rw---- 1 root uucp 188, 0 Sep 14 14:14 /dev/ttyUSB0
             ```
          2. the user group you need to add yourself to is after `root`. In the example above, it is `uucp`, but it may also be `dialout` or something else
          3. run `sudo usermod -aG [group] $USER`
          4. run `newgrp [group]`
          5. to check that you are in the group, run `groups $USER`

             *example output:*
             ```bash
             evinl : evinl wheel uucp docker
             ```
    8. The example should now be successfully flashed. In order to check, run `esptool --port /dev/[ttyUSB0] chip-id`. *Note: esptool is automatically installed with zephyr/west (I think). If it is not, install it with `pip`
    9. If everything works, the output should show at the very least the name of the MCU (for the ESP32-S3, it will say something like `Connected to ESP32-S3 on /dev/[ttyUSB0]` and also warn that it has no chip id. This means that the USB port is properly communicating.
    10. You can also run `read-flash` instead of `chip-id` if you want

9. Test Debugger
     1. Ensure you are plugged into the UART connection (the debugger)
     2. `just run-esp32`
     3. Once the board is done flashing, press the `reset` button
     4. Your program in app/src/main.c should run and print `Hello World`.


