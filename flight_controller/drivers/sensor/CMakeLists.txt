# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory_ifdef(CONFIG_EXAMPLE_SENSOR example_sensor)
add_subdirectory_ifdef(CONFIG_LIDAR_LITE_V4 lidar_lite_v4) 
add_subdirectory_ifdef(CONFIG_BNO055 bno055)