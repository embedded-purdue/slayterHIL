#-------------------------------------------------------------------------------
# Zephyr Example Application
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(my_zephyr_app)

project(app LANGUAGES C)

file(GLOB THREAD_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/threads/*.c)
target_sources(app PRIVATE src/main.c ${THREAD_SRCS})

# All #includes to files in src directory should be relative to src directory root
zephyr_include_directories(src)
