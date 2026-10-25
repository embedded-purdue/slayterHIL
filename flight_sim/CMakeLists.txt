cmake_minimum_required(VERSION 3.22)
project(flight_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

find_package(Eigen3 REQUIRED NO_MODULE)
# Prefer config targets so modern Protobuf brings its Abseil dependencies.
find_package(Protobuf CONFIG QUIET)
if(NOT Protobuf_FOUND)
  find_package(Protobuf REQUIRED)
endif()
# nlohmann_json vendored as include/json.hpp (single-header amalgamation)


# # DEBUG
# get_target_property(EIGEN_INC Eigen3::Eigen INTERFACE_INCLUDE_DIRECTORIES)
# message(STATUS "Eigen include path: ${EIGEN_INC}")

# ---- Proto setup -------------------------------------------------------------

set(PROTO_DIR ${CMAKE_SOURCE_DIR}/../shared/proto)
set(PROTO_FILES "${PROTO_DIR}/sensor_data.proto")

set(GEN_PROTO_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GEN_PROTO_DIR})

add_library(proto_lib STATIC)

protobuf_generate(
  TARGET proto_lib
  LANGUAGE cpp
  PROTOS ${PROTO_FILES}
  IMPORT_DIRS ${CMAKE_SOURCE_DIR}/../ ${PROTO_DIR}
  PROTOC_OUT_DIR ${GEN_PROTO_DIR}
  APPEND_PATH OFF
)

get_target_property(_srcs proto_lib SOURCES)
message(STATUS "proto_lib sources: ${_srcs}")

target_link_libraries(proto_lib PUBLIC protobuf::libprotobuf)

target_include_directories(proto_lib PUBLIC
  ${GEN_PROTO_DIR}
  ${PROTO_DIR}
)

# ---- Executables ------------------------------------------------------------

file(GLOB SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
# Legacy sender uses a different schema and is not part of this executable.
list(FILTER SRC_FILES EXCLUDE REGEX ".*/spi_new_test\\.cpp$")
# SPI files use Linux kernel headers — exclude on non-Linux platforms
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(FILTER SRC_FILES EXCLUDE REGEX ".*/spi_linux\\.cpp$")
endif()

# Keep entry points separate: both define main().
list(FILTER SRC_FILES EXCLUDE REGEX ".*/(main|old_main)\\.cpp$")
add_executable(${PROJECT_NAME} src/old_main.cpp ${SRC_FILES})
# Hardware sender is opt-in; the default build remains the desktop simulation.
add_executable(old_main_spi_test EXCLUDE_FROM_ALL src/old_main.cpp ${SRC_FILES})
target_compile_definitions(old_main_spi_test PRIVATE
  FLIGHT_SIM_DEFAULT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../tests/flightpaths/test.txt")
target_include_directories(old_main_spi_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(old_main_spi_test PRIVATE proto_lib Eigen3::Eigen)
target_compile_definitions(${PROJECT_NAME} PRIVATE
  FLIGHT_SIM_DEFAULT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../tests/flightpaths/test.txt")

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    proto_lib
    Eigen3::Eigen
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)


include(CTest)
if(BUILD_TESTING)
  add_executable(sensor_frame_test tests/sensor_frame_test.cpp src/serialize.cpp)
  target_include_directories(sensor_frame_test PRIVATE include)
  target_link_libraries(sensor_frame_test PRIVATE proto_lib Eigen3::Eigen)
  add_test(NAME sensor_frame_test COMMAND sensor_frame_test)
endif()
