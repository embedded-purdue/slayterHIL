#-------------------------------------------------------------------------------
# Zephyr Example Application
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(ZEPHYR_EXTRA_MODULES "${CMAKE_SOURCE_DIR}/../modules/spi_test")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(spi_module_app)

project(app LANGUAGES C)

target_sources(app PRIVATE src/main.c)