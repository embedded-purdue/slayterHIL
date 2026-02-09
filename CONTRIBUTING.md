# Contributor Documentation

The following is documentation related to information for every SlayterHIL subteam. Technical information regarding to contributing to a particular part of the codebase is outlined, as well as a member registry and branch usage per subteam.

---

# Table of Contents

- [Flight Simulation](#flight-simulation)
  - [Development and Branching](#development-and-branching)
  - [Team members](#team-members)
  - [SSH Connection for RPI](#ssh-connection-for-rpi)
  - [Technical Documentation](#technical-documentation)

---

# Flight Simulation

*Last Updated: February 9, 2026*

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

To add your device to the Tailscale network, you can use the SlayterHil Google account and password that are seen below to sign in inside the Tailscale website.

**EMAIL:** slayterhil2026@gmail.com
**PASSWORD:** closedloopotw!

The RPI, which is named `raspi-5-orcheslayter` at the time of writing, can be accessed through the `flight_sim` user. Information to SSH can be seen below.

**SSH**: ssh flight_sim@raspi-5-orcheslayter
**PASSWORD**: slayterHIL5

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


