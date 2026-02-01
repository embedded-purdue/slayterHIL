# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Out-of-tree drivers for existing driver classes
# Out-of-tree drivers for flight_controller

cmake_minimum_required(VERSION 3.20.0)
project(drivers)

if(CONFIG_LIDAR_LITE_V4)
  add_subdirectory(sensor/lidar_lite_v4)
endif()