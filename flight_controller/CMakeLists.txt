cmake_minimum_required(VERSION 3.20.0)

# Optional: point to esptool.py (Zephyr sometimes looks for it)
set(ESPTOOL_PY_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/hal/espressif/components/esptool_py/esptool/esptool.py
    CACHE FILEPATH "esptool.py path override"
)


# --- Load Zephyr ---
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(flight_controller)

# --- Diagnostics ---
message(STATUS "==> Zephyr base: ${ZEPHYR_BASE}")
message(STATUS "==> Current source dir: ${CMAKE_CURRENT_SOURCE_DIR}")

# --- Add custom LiDAR driver ---
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/drivers/sensor/lidar_lite_v4)

# --- Add main app source explicitly ---
set(APP_MAIN ${CMAKE_CURRENT_SOURCE_DIR}/app/src/main.c)
if(EXISTS ${APP_MAIN})
    message(STATUS "==> Found app main at: ${APP_MAIN}")
else()
    message(FATAL_ERROR "❌ main.c not found at expected path: ${APP_MAIN}")
endif()

target_sources(app PRIVATE ${APP_MAIN})
