# SlayterHiL - DUT

# Setup

0. Install prerequisites

   - [Python3](https://www.geeksforgeeks.org/python/download-and-install-python-3-latest-version/)

   - Complete **only** the following sections of the [Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#getting-started-guide).

     - [Install Dependencies](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies)
     - Skip _Get Zephyr and install Python dependencies_
     - [Install the Zephyr SDK](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-the-zephyr-sdk)

   - [Just command runner](https://github.com/casey/just?tab=readme-ov-file#installation) - This is used like a makefile as a hotkey for all of our project-specific commands

1. Clone the repo
2. `python -m venv venv`
3. `source venv/bin/activate`
4. `pip install west`
5. Update the workspace

```
cd flight_controller
west update
west packages pip --install
```

6. Make sure it works:
   `just run-sim`

# What is implemented so far

What is implemented

- BNO-055 driver
- LiDAR-Lite v4 driver
- PID controllers for (pitch, roll, altitude)
  - The constants are not verified and can’t be verified without the simulation working or an actual working drone.
- RC commands over UART(will talk more about the actual commands) -> sets targets
- PWM output to “motors” - made them leds.
- State machine (implemented not tested)

## Hardware & Pinout

**Target Board**: `esp32s3_devkitc/esp32s3/procpu`
**pin config:**: `app/boards/esp32s3_devkitc.overlay`

| Function               | Signal        | GPIO            | Notes                                                 |
| ---------------------- | ------------- | --------------- | ----------------------------------------------------- |
| **I²C0** (BNO055)      | SDA           | **GPIO8**       | 100 kHz (`I2C_BITRATE_STANDARD`)                      |
|                        | SCL           | **GPIO9**       |                                                       |
|                        | BNO055 INT    | **GPIO12**      | active-high, pull-down                                |
|                        | BNO055 addr   | `0x28`          |                                                       |
| **I²C1** (LiDAR)       | SDA           | **GPIO4**       | 100 kHz                                               |
|                        | SCL           | **GPIO5**       |                                                       |
|                        | LiDAR addr    | `0x62`          |                                                       |
| **UART1** (RC cmds)    | TX            | **GPIO16**      | 115200 8N1                                            |
|                        | RX            | **GPIO17**      | pulled-up **input** (see gotcha #7)                   |
| **PWM motors** (LEDC0) | CH0           | **GPIO6**       | alias `led0`, mix index `BOTTOM_LEFT` (0)             |
|                        | CH1           | **GPIO7**       | alias `led1`, mix index `TOP_LEFT` (1)                |
|                        | CH2           | **GPIO1**       | alias `led2`, mix index `TOP_RIGHT` (2)               |
|                        | CH3           | **GPIO2**       | alias `led3`, mix index `BOTTOM_RIGHT` (3)            |
| **Status LED**         | proximity LED | **GPIO13**      | alias `led4`, GPIO (not PWM); lit when LiDAR < 300 mm |
| **Button**             | button0       | **GPIO14**      | active-high, pull-down (unused in app)                |
| **Console**            | UART0 TX/RX   | GPIO43 / GPIO44 | board default, via CP2102 USB-UART bridge, 115200     |

### LED/GPIO map

The mix indices in `main.c` are **not** in numeric order:

| Motor       | `main.c` index     | LED alias | LEDC ch | GPIO  |
| ----------- | ------------------ | --------- | ------- | ----- |
| Front-Left  | `TOP_LEFT` = 1     | led1      | 1       | GPIO7 |
| Front-Right | `TOP_RIGHT` = 2    | led2      | 2       | GPIO1 |
| Back-Left   | `BOTTOM_LEFT` = 0  | led0      | 0       | GPIO6 |
| Back-Right  | `BOTTOM_RIGHT` = 3 | led3      | 3       | GPIO2 |

## 3. Sensor wiring

**BNO055**

- `PSO -> gnd`
- `PS1 -> gnd`
- `ADR -> gnd`
- `VIN -> 3v3`
- `RST -> 3v3
  - Dont tie this to gnd it is `active-low`

### LiDAR

![alt text](<Screenshot 2026-09-06 at 01-20-43.png>)

- `MAKE SURE VIN for LiDAR goes on the 5v instead of the 3v3`

## 4. Software architecture

### Threads

| Thread                               | Prio | Stack | Role                                                      |
| ------------------------------------ | ---- | ----- | --------------------------------------------------------- |
| `main`                               | —    | 4096  | Init hardware, start threads, then returns                |
| `state_machine_thread`               | 1    | 2048  | Consumes `event_msgq`; **handlers stubbed**               |
| `bno_thread` (`bno_read_thread`)     | 2    | 4096  | Polls BNO055 @ ~25 Hz → `bno_msgq` (gated on `bno_ready`) |
| `bno_consumer`                       | 3    | 4096  | **The 20 Hz control loop** — 3 PIDs + mix + telemetry     |
| `lidar_thread` (`lidar_read_thread`) | 4    | 2048  | Waits on `lidar_sem` (33 Hz timer) → `lidar_msgq`         |
| `lidar_consumer`                     | 4    | 2048  | Drains `lidar_msgq` → publishes altitude, proximity LED   |
| `uart_consumer`                      | 5    | 2048  | Drains `uart_rx_msgq` → adjusts setpoints                 |

## 5. Control system

`bno_consumer` (`main.c`) runs one 20 Hz loop with three PIDs (`pid.c`).

| PID      | Measured            | Setpoint (target)                                | Gains Kp/Ki/Kd, I-clamp  |
| -------- | ------------------- | ------------------------------------------------ | ------------------------ |
| Pitch    | BNO `-eul.pitch`    | RC-commanded, default **0°**                     | 8.0 / 0.5 / 2.0, ±20     |
| Roll     | BNO `-eul.roll`     | RC-commanded, default **0°**                     | 10.0 / 0.5 / 2.0, ±20    |
| Altitude | LiDAR `distance_mm` | seeded to **first LiDAR reading**, RC-adjustable | 0.05 / 0.001 / 0.02, ±30 |

Yaw is read and printed but **not controlled** (no yaw PID).

**Motor mix** (`base = 50`, `collective = base + altitude_out`):

```
FL = collective + pitch_out + roll_out
FR = collective - pitch_out + roll_out
BL = collective + pitch_out - roll_out
BR = collective - pitch_out - roll_out      (each clamped 0..100)
```

### RC commands (UART1, one raw byte each)

| Key       | Effect                  | Step / limit               |
| --------- | ----------------------- | -------------------------- |
| `L` / `R` | roll setpoint ∓ / ±     | ±2°, clamp ±15°            |
| `U` / `D` | pitch setpoint ± / ∓    | ±2°, clamp ±15°            |
| `A` / `X` | altitude setpoint + / − | ±100 mm, clamp 200–3000 mm |

Altitude hold engages on the **first** LiDAR sample (setpoint seeded to current height).

---

## 6. File map

```
flight_controller/
  app/
    src/
      main.c          Threads, control loop (bno_consumer), mix, telemetry, I2C scan
      control.c/.h    Mutex-protected shared setpoints + latest altitude
      pid.c/.h        Generic PID (pid_ctrl_t): init / update / reset
      bno.c/.h        BNO055 app wrapper: init, read thread, bno_get
      lidar.c/.h      LiDAR app wrapper: read thread, lidar_get
      uart.c/.h       UART1 RX ISR → uart_rx_msgq
      ledCtrl.c/.h    PWM "motor" output (set_led_intensity)
      state_machine.c Event FSM — STUBBED
      imu.c/.h        Legacy MPU6050 — UNUSED (not in CMake)
    boards/esp32s3_devkitc.overlay   Pin/bus config (source of truth)
    prj.conf          Kconfig (sensors, PWM, UART IRQ, diagnostics)
    CMakeLists.txt    Source list (add new .c files here)
  drivers/sensor/
    bno055/           Custom BNO055 driver (register/SHTP-free, register map)
    lidar_lite_v4/    Custom LiDAR-Lite V4 driver (I2C poll)
  justfile            Build / flash / monitor recipes
```

## Running the code:

**Make sure you change the just file esp32 port**

### To find the port the esp32 is connected to:

#### On mac:

`ls /dev/tty.*`

#### On windows:

**Lowk no idea on how to do windows: run the WSL linux terminal and do:**
`ls /dev/ttyS*`

```
ESP_PORT         := "<YOUR PORT HERE>"
```

```sh
just build-esp32     # pristine build for esp32s3_devkitc + the overlay
just flash-esp32     # flash build/ to the board
just monitor-esp32   # serial monitor @ 115200
just run-esp32       # build + flash + monitor
python -m serial.tools.miniterm "<PORT>" 115200 # lowk just use this on my machine not sure setup for it try it.

```

## 9. Debugging & diagnostics

These are **intentional and kept on purpose** — this is how you debug the board.

### Build-time diagnostics (`app/prj.conf`)

| Kconfig                                                                    | What it does                                                                                                                                             | When it helps                                                                                 |
| -------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------- |
| `CONFIG_STACK_SENTINEL=y`                                                  | Puts a guard word at each stack's end, checked on every context switch. A stack overflow becomes a **named fatal message** instead of silent corruption. | Any random crash / `LoadProhibited` with a garbage pointer — proves or denies stack overflow. |
| `CONFIG_THREAD_NAME=y`                                                     | Attaches names to threads so crash dumps and the analyzer show real names, not `(unknown)`.                                                              | Reading any crash dump or analyzer output.                                                    |
| `CONFIG_THREAD_STACK_INFO=y`                                               | Records per-thread stack bounds (required by the analyzer).                                                                                              | Prerequisite for stack-usage reporting.                                                       |
| `CONFIG_THREAD_ANALYZER=y` (+ `_USE_PRINTK`, `_AUTO`, `_AUTO_INTERVAL=20`) | Spawns a thread that prints **every thread's stack high-water mark + CPU %** every 20 s.                                                                 | Confirming stack headroom; catching a thread creeping toward its limit.                       |

Reading an analyzer line — `STACK: unused 2908 usage 1188 / 4096 (29 %)` means 1188 of
4096 bytes used. If any thread nears 100 %, enlarge its stack: the app threads use the
`*_STACK_SIZE` defines at the top of `main.c`; Zephyr's own threads use their Kconfigs
(e.g. `CONFIG_LOG_PROCESS_THREAD_STACK_SIZE` for `logging`).

Want a quieter console for a demo? **Raise** `CONFIG_THREAD_ANALYZER_AUTO_INTERVAL`
rather than deleting the config.

### In-firmware diagnostics (always on, in the app code)

- **I²C bus scanner** — `i2c_scan()` in `main.c`, runs at boot. Probes every address on
  both buses and prints which ones ACK. Distinguishes "sensor alive at the right address"
  (miswired) from "nothing responds" (dead / unpowered / no clock line). Expect `0x28`
  (BNO055) and `0x62` (LiDAR). This is what caught the BNO085-instead-of-BNO055 swap.
- **Telemetry line** (~2 Hz, in `bno_consumer`) — live P/R/Y, setpoints, altitude, `rx:`
  count, and the motor mix. Integer-formatted via `fx2()` on purpose (no `%f` — see #6).
- **`rx:` counter** — bytes accepted by UART1. Fastest way to localize a UART fault:
  `0` while sending = nothing reaching GPIO17 (wiring / pin config); climbing but no
  setpoint change = wrong baud or non-command bytes.
