Stripped down and optimized version of Zephyr's [example application](https://github.com/zephyrproject-rtos/example-application)

## Quick Start

### Prerequisites

- [Zephyr SDK](https://docs.zephyrproject.org/latest/getting_started/index.html)
- [West tool](https://docs.zephyrproject.org/latest/guides/west/index.html)
- [Just command runner](https://github.com/casey/just)
- Python environment with pyenv (recommended)

### Setup Instructions

```bash
# Initialize workspace for the lidar demo app

west init -m https://github.com/alexayl/Zephyr-App-Base --mr main zephyr_base_app

cd projects/zephyr-base-app-workspace

pyenv local zephyr-313 # if using pyenv

west update

cd zephyr-base-app

west packages pip --install

just init

just build-esp32
```

### Building and Flashing

#### For ESP32-S3:
```bash
# Build the application
just build-esp32

# Flash to device
just flash-esp32

# Monitor serial output
just monitor-esp32

# Or do all in one command
just run-esp32
```

## Usage

[tbd]
## License

This project is licensed under the Apache License 2.0. See the LICENSE file for details.

---

*Based on my [Zephyr starter app template](https://github.com/alexayl/zephyr-app-base)*
