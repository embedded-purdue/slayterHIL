cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app LANGUAGES C)

file(GLOB THREAD_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/threads/*.c)
target_sources(app PRIVATE src/main.c src/hil_diag.c ${THREAD_SRCS})

zephyr_include_directories(src)

zephyr_include_directories(${ZEPHYR_BASE}/../modules/lib/nanopb)

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)
FILE(GLOB proto_sources ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/proto/*.proto)
zephyr_nanopb_sources(app ${proto_sources})
