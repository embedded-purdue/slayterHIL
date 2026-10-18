cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app LANGUAGES C)

# Only build the MVP receive/emulation path. Legacy PWM/RC experiments are
# incomplete and are not used by this application.
target_sources(app PRIVATE src/main.c src/hil_diag.c src/sensor_frame.c
  src/threads/scheduler_test.c src/threads/sensor_emulation_test.c
  src/threads/imu_emulator_test.c src/threads/lidar_emulator_test.c)

zephyr_include_directories(src)

zephyr_include_directories(${ZEPHYR_BASE}/../modules/lib/nanopb)

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)
set(proto_sources ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/proto/sensor_data.proto)
zephyr_nanopb_sources(app ${proto_sources})
