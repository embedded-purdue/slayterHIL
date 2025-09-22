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

# make src/ available for includes
zephyr_include_directories(src)

# Protobuf
list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)

FILE(GLOB proto_sources ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/proto/*.proto)
zephyr_nanopb_sources(app ${proto_sources})