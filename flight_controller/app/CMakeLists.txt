#-------------------------------------------------------------------------------
# Zephyr Example Application
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(my_zephyr_app)

project(app LANGUAGES C)
target_include_directories(app  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_sources(app PRIVATE src/main.c src/state_machine.c src/imu.c src/ledCtrl.c)