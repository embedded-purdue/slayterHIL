# Flight Sim Migration Guide

## Scope

`flight_sim/` currently contains three mostly separate pieces:

1. The main C++ simulator in `src/` and `include/`.
2. SPI/protobuf transport code used to package simulator output for hardware.
3. Two visualization experiments under `graphics/`:
   - an SFML C++ viewer
   - a Python/Ursina mock visualizer

The main executable is the simulator built from `flight_sim/CMakeLists.txt`. The graphics code is built separately from `flight_sim/graphics/CMakeLists.txt`.

## Repository Layout

### Core simulator

- `include/flight_sim.hpp`
  Umbrella header used by most `.cpp` files. It pulls in Eigen, controllers, physics types, IMU code, SPI abstractions, JSON, and Linux SPI headers on Linux.

- `src/main.cpp`
  Current simulator entry point. This is the file that wires together:
  - RC path input
  - waypoint generation
  - controllers
  - drone dynamics
  - IMU simulation
  - CSV logging

- `include/physics_body.hpp`
  Abstract base class for simulated bodies. Defines common state:
  - `mass`
  - `position`
  - `velocity`
  - `acceleration`
  - `total_force`
  - `orientation`

- `include/rigid_body.hpp`, `src/rigid_body.cpp`
  Main dynamics implementation. `RigidBody` extends `PhysicsBody` with:
  - body-frame inertia
  - torque accumulation
  - angular velocity
  - RK4 integration
  - basic AABB-style collision helpers

- `include/drone.hpp`, `src/drone.cpp`
  Composite drone object made from five `RigidBody`s:
  - central body
  - four motor bodies

  This is a convenience wrapper, not a physically constrained multibody model. It forwards forces to all child bodies and derives aggregate mass/torque from them.

- `include/positionController.hpp`, `src/positionController.cpp`
  Proportional position controller. Converts position error into target velocity.

- `include/velocityController.hpp`, `src/velocityController.cpp`
  PID-like velocity controller with output limiting. Converts target velocity into force.

- `include/PIDcalculator.hpp`, `src/PIDcalculator.cpp`
  Older controller implementation. Present in the build, but `src/main.cpp` currently uses `positionController` + `velocityController` instead.

- `include/imu_generation.hpp`, `src/imu_generation.cpp`
  Converts force/torque into simulated IMU outputs in the packed byte-oriented format used elsewhere in the project.

- `include/rc_parser.hpp`, `src/RC_Parser.cpp`
  Reads simple RC commands from `../../tests/flightpaths/`.

- `include/serialize.hpp`, `src/serialize.cpp`
  Manual serializer for `imu_data_t` into a fixed byte buffer. This is separate from the protobuf path.

### SPI and protobuf

- `include/spi_interface.hpp`
  Platform-agnostic SPI interface with `init()` and `transmit()`.

- `src/spi_linux.cpp`
  Linux implementation using `/dev/spidev0.0` and `ioctl`.

- `src/spi_stub.cpp`
  Non-hardware stub. Appends transmitted bytes to `spi_payload.log`.

- `include/spi_new_test.hpp`, `src/spi_new_test.cpp`
  Newer SPI/protobuf experiment. Builds a `DeviceUpdatePacket` from `shared/proto/node_imu.proto`, serializes it with protobuf, and sends it over Linux SPI.

### Visualization

- `graphics/src/main.cpp`
  Separate SFML viewer. Not integrated with the main simulator loop.

- `graphics/src/mesh.cpp`
  Contains mesh, camera, projection, and drawing code. Included directly by `graphics/src/main.cpp`.

- `graphics/graphics/visualizer.py`
  Separate Python/Ursina visualization prototype with mock telemetry.

### Legacy / non-authoritative code

- `legacy/`
  Old experiments. Not part of the current build.

- `tests/spi_output_test.cpp`
  Standalone SPI test example. Not wired into CMake.

- `include/collection.hpp`, `include/joint.hpp`, `include/simulation.hpp`
  Early scaffolding. Not central to the current executable.

## Build and Compile

## Main simulator

The main simulator is configured by `flight_sim/CMakeLists.txt`.

### Requirements

- CMake `>= 3.22`
- C++23 compiler
- Eigen3
- Protobuf
- Access to `../shared/proto/*.proto`

### What the build does

1. Sets `CMAKE_CXX_STANDARD 23`.
2. Finds `Eigen3`.
3. Finds `Protobuf`.
4. Generates C++ protobuf sources from `../shared/proto/*.proto`.
5. Builds a static `proto_lib`.
6. Builds executable `flight_sim` from all `src/*.cpp`.
7. On non-Linux systems, excludes:
   - `src/spi_linux.cpp`
   - `src/spi_new_test.cpp`

Important consequence: every `.cpp` file in `src/` is globbed into the executable. If you add a new `.cpp` to `src/`, it will be compiled automatically. If you add experimental files there with duplicate symbols or `main()`, the build will break.

### Recommended commands

From `flight_sim/`:

```bash
cmake -S . -B build
cmake --build build
```

The repo also includes `setup.sh`, but it is minimal:

```bash
mkdir build
cd build
cmake ..
make
```

Use the explicit `cmake -S/-B` flow instead. `setup.sh` assumes `build/` does not already exist and does not expose configuration options.

### Protobuf dependency

The simulator depends on `../shared/proto`, relative to `flight_sim/`. If that shared directory is moved, renamed, or split out, the CMake proto generation step must be updated.

Generated protobuf files are emitted under:

- `flight_sim/build/generated/`

Do not hand-edit generated files there.

## Graphics build

The SFML viewer under `flight_sim/graphics/` is a separate CMake project.

Requirements:

- CMake `>= 3.28`
- C++17 compiler
- network access at configure time, because it uses `FetchContent` to clone SFML `3.0.1`

Commands:

```bash
cd flight_sim/graphics
cmake -S . -B build
cmake --build build
```

This produces a separate executable named `main`. It is not linked to the simulator executable.

## How the Main Simulator Works

## High-level flow

`src/main.cpp` currently does the following:

1. Creates an `ImuSimulator`.
2. Reads RC instructions from `tests/flightpaths/test.txt` through `rc_read("test.txt")`.
3. Converts each instruction vector into a waypoint spaced `0.5` seconds apart.
4. Constructs a `Drone` from five `RigidBody` instances.
5. Creates a ground `RigidBody` with bounds, though collision handling is not active in the loop.
6. Creates:
   - a position controller
   - a velocity controller
7. Runs a fixed-step simulation loop at `1/60` seconds for `30` seconds.
8. On each step:
   - advances waypoint target when the time threshold is reached
   - applies gravity manually
   - computes target velocity from position error
   - computes control force from velocity error
   - adds gravity compensation on `z`
   - applies force to the drone
   - advances drone state
   - derives IMU output from force and net torque
   - prints state to stdout
   - writes a CSV row to `pid_tuning.csv`

## Control path

The current controller stack is:

`RC commands -> waypoint target -> positionController -> target velocity -> velocityController -> force -> Drone`

Details:

- `positionController::compute()` is purely proportional:
  `targetVelocity = kp .* (desiredPos - currentPos)`

- `velocityController::compute()` computes:
  - proportional term from velocity error
  - derivative term from error delta
  - integral accumulation only when output norm is below `maxForce`
  - norm clamp to `maxForce`

`PIDcalculator` is not in the active control path.

## Dynamics path

`RigidBody` owns the actual integration logic.

### Force / torque accumulation

- `applyForce()` adds to `total_force` in world frame.
- `applyTorque()` adds to `total_torque_body` in body frame.

### Integration

`RigidBody::update(dt)`:

1. Computes state derivatives.
2. Performs RK4 integration for:
   - position
   - linear velocity
   - angular velocity
   - orientation quaternion
3. Normalizes the quaternion.
4. Clears force and torque accumulators.

### Drone wrapper behavior

`Drone` is not a single rigid body with motor arm geometry. It is a wrapper around five independent `RigidBody`s. Current behavior:

- `Drone::applyForce()` applies the same force to all five bodies.
- `Drone::update()` updates all five bodies independently.
- `getPosition()` and `getVelocity()` return the central body values only.
- aggregate mass / net force / net torque / angular velocity are computed by summing child bodies.

This means the drone model is only partially coherent physically. It is adequate for controller prototyping, but not yet a true constrained rigid multibody model.

## IMU path

`ImuSimulator` takes either:

- scalar force/torque components, or
- `Eigen::Vector3f force`, `Eigen::Vector3f torque`

It computes:

- linear acceleration as `force / mass`
- angular rates by integrating `torque / I`
- Euler angles by integrating angular rates

Then it packs the results into `imu_data_t`, where each component is stored as two bytes (`lsb`, `msb`) after scaling.

Current limitations:

- it tracks Euler angles, not quaternion state
- it assumes scalar moment of inertia
- it does not derive IMU data from the rigid body orientation directly

## RC input path

`rc_read(test_name)` opens:

`../../tests/flightpaths/<test_name>`

relative to the process working directory.

It reads dot-delimited command chunks such as:

- `L`
- `R`
- `F`
- `B`
- `U`
- `D`
- combinations like `UF`

Each command is mapped into a unit `Eigen::Vector3d`.

Important consequence: the program assumes it is launched from a directory where that relative path resolves correctly. If you run the executable from an unexpected working directory, RC file loading will fail.

## Logging

`src/main.cpp` writes `pid_tuning.csv` in the process working directory.

Logged fields:

- `Time`
- `X`
- `Y`
- `Z`
- `errorZ`
- control output `Z`

This is lightweight tuning output, not a general telemetry system.

## SPI / Protobuf Path

There are two serialization paths in the directory. Keep them separate conceptually.

## Path 1: manual byte serialization

`src/serialize.cpp` writes `imu_data_t` into a fixed byte buffer with hard-coded field tags and byte positions.

Use this only if the receiver explicitly expects this exact packed format.

## Path 2: protobuf serialization

`src/spi_new_test.cpp` uses generated protobuf code from:

- `shared/proto/node_imu.proto`

It builds a `DeviceUpdatePacket`, serializes it with protobuf, and sends the resulting bytes over SPI.

This is the more maintainable path if both sides of the link can evolve together.

## Linux SPI implementation

`src/spi_linux.cpp` defines a concrete `SpiLinux` class implementing `SpiInterface`.

Notes:

- the class lives only in the `.cpp`; there is no public header for constructing it elsewhere
- it defaults to `/dev/spidev0.0`
- it uses Linux kernel SPI headers
- it is excluded from non-Linux builds by CMake

If you want SPI to be used by the main simulator, the current code needs integration work. Right now, `src/main.cpp` computes IMU data but does not hand that data to a `SpiInterface`.

## How to Implement New Things

## Add a new controller

Recommended approach:

1. Add a header in `include/`.
2. Add the implementation in `src/`.
3. Instantiate it in `src/main.cpp`.
4. Replace or extend the current control chain.

Guidelines:

- Keep controller APIs stateless where possible, or clearly isolate integrator state.
- Use `Eigen::Vector3d` consistently for world-space translational control.
- Keep units explicit. The current code assumes SI-like units.

If you are replacing the active stack, edit the block in `src/main.cpp` that currently does:

- gravity application
- position controller compute
- velocity controller compute
- gravity compensation
- `drone->applyForce(force)`

## Add a new flight path format

If the new team wants richer trajectories, do not extend the existing RC dot-delimited format unless simplicity is required.

Current options:

- keep `rc_read()` for simple directional tests
- add a JSON trajectory loader using the vendored `include/json.hpp`
- add timestamped waypoint loading directly

Best insertion point:

- replace the `rc_read("test.txt")` + waypoint conversion block in `src/main.cpp`

If using JSON, note that there are already generated test artifacts in `tests/flightpaths/*.json`, but the current executable does not consume them.

## Add new physical effects

For wind, drag, thrust models, disturbances, or better torques:

1. Compute the new force/torque each frame in `src/main.cpp`, or
2. move force assembly into a dedicated simulation/orchestrator class if the loop becomes crowded.

Current state:

- `Simulation` in `include/simulation.hpp` is placeholder scaffolding only
- environmental effects are not centralized

Recommended direction:

- keep `RigidBody` focused on integration
- keep force model code outside `RigidBody`
- accumulate all world-frame forces before calling `update(dt)`

If the model becomes more complex, introduce a real simulation coordinator instead of continuing to expand `src/main.cpp`.

## Add true multibody drone dynamics

If the goal is physical fidelity, `Drone` should be reworked.

Current limitations to address:

- the same force is applied to every child body
- there are no constraints enforcing arm geometry
- the aggregate drone state is not integrated as one physical body
- torques are not generated from rotor thrust offsets

Two viable directions:

1. Simplify to one `RigidBody` for the entire drone and compute torques from rotor commands analytically.
2. Build a true constrained multibody system and retire the current wrapper behavior.

For most control and HIL work, option 1 is the better tradeoff.

## Add new telemetry fields to hardware output

If sending data over protobuf:

1. Update the relevant `.proto` file in `shared/proto/`.
2. Re-run CMake configure/build so protobuf code is regenerated.
3. Update `src/spi_new_test.cpp` to populate the new fields.
4. Update the receiver side to decode the new schema.

If sending data through the manual serializer:

1. Update `imu_data_t` or surrounding data structs.
2. Update `src/serialize.cpp`.
3. Update the receiver to match the exact byte layout.

Prefer protobuf unless there is a hard protocol constraint.

## Add a new executable

Do not drop an extra `main()` into `src/`, because `flight_sim/CMakeLists.txt` compiles every `.cpp` in that directory into the same executable.

Instead:

1. create a dedicated target in `flight_sim/CMakeLists.txt`, or
2. move experimental programs to a separate directory and wire them explicitly

This matters for tests, tools, benchmarks, and one-off hardware utilities.

## Add tests

There is currently no proper CTest or unit test setup in `flight_sim/CMakeLists.txt`.

Recommended first step:

1. add `enable_testing()`
2. choose a framework or write simple executable-based tests
3. keep deterministic controller/dynamics checks separate from hardware SPI tests

Low-friction test targets:

- controller response tests
- `RigidBody` integrator regression tests
- RC parser tests
- protobuf serialization sanity tests

Avoid coupling hardware SPI tests to the default simulator build.

## Add visualization linked to live sim state

The current viewers are disconnected from the main simulation.

To integrate visualization cleanly:

1. define a telemetry/state snapshot struct
2. publish it from the sim loop
3. let the viewer consume that state through:
   - file output
   - socket/UDP
   - shared memory
   - direct in-process integration

Do not build new features on the assumption that `graphics/` already reflects the live simulator state. It does not.

## Current Risks and Known Technical Debt

- `src/main.cpp` is the real orchestrator; `Simulation` is not used.
- `Drone` is only a partial abstraction and is not a high-fidelity physics model.
- `RigidBody::isColliding()` uses crude bounds checks and does not handle orientation correctly.
- `rc_read()` depends on fragile relative paths.
- `src/spi_linux.cpp` hides the concrete class inside the implementation file, which limits reuse.
- `src/spi_new_test.cpp` is still an experiment, not integrated production flow.
- `tests/spi_output_test.cpp` is not part of the build.
- `graphics/` is separate and currently prototype-level.
- `flight_sim/README.md` is stale and should not be treated as authoritative.

## Recommended Near-Term Cleanup

If the next team wants a stable base before adding features, the highest-value cleanup is:

1. Move sim orchestration out of `src/main.cpp` into a real `Simulation` implementation.
2. Replace the current `Drone` wrapper with either:
   - one physically consistent rigid body, or
   - a real constrained multibody model.
3. Standardize one telemetry path:
   - protobuf for external transport
   - a typed in-process/state-snapshot path for internal tooling
4. Make all file paths configurable instead of hard-coded relative strings.
5. Add real tests and separate hardware-specific executables from the default simulator target.

## Minimal Working Mental Model

If you need the shortest accurate model of the current system, it is this:

- The simulator reads simple directional commands from a test file.
- Those commands become time-based waypoints.
- A P controller converts waypoint error into target velocity.
- A velocity PID converts target velocity into force.
- A `RigidBody`-based drone surrogate integrates the motion at 60 Hz.
- An IMU simulator converts force/torque into packed sensor-like output.
- SPI/protobuf code exists, but is not yet integrated into the main sim loop.
- Visualization exists, but as separate prototypes.
