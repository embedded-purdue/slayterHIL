cmake_minimum_required(VERSION 3.22)
project(flight_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Eigen3 REQUIRED NO_MODULE)
find_package(Protobuf REQUIRED) # provides protobuf::libprotobuf and (often) protobuf::protoc
# absl is a required dep of protobuf 6.x; add its include path globally
foreach(_prefix /opt/homebrew /usr/local)
  if(EXISTS "${_prefix}/include/absl")
    include_directories(SYSTEM "${_prefix}/include")
    message(STATUS "absl headers added from ${_prefix}/include")
    break()
  endif()
endforeach()
# nlohmann_json vendored as include/json.hpp (single-header amalgamation)


# # DEBUG
# get_target_property(EIGEN_INC Eigen3::Eigen INTERFACE_INCLUDE_DIRECTORIES)
# message(STATUS "Eigen include path: ${EIGEN_INC}")

# ---- Proto setup -------------------------------------------------------------

set(PROTO_DIR ${CMAKE_SOURCE_DIR}/../shared/proto)
file(GLOB PROTO_FILES "${PROTO_DIR}/*.proto")

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
# SPI files use Linux kernel headers — exclude on non-Linux platforms
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(FILTER SRC_FILES EXCLUDE REGEX ".*/spi_linux\\.cpp$")
  list(FILTER SRC_FILES EXCLUDE REGEX ".*/spi_new_test\\.cpp$")
endif()

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    proto_lib
    Eigen3::Eigen
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

