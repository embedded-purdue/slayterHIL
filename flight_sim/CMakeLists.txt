cmake_minimum_required(VERSION 3.22)
project(flight_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Eigen3 3.4 REQUIRED NO_MODULE)
find_package(Protobuf REQUIRED) # provides protobuf::libprotobuf and (often) protobuf::protoc
find_package(nlohmann_json REQUIRED) # <-- add this (apt: nlohmann-json3-dev)

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
  IMPORT_DIRS ${CMAKE_SOURCE_DIR}/../
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

add_executable(json_test utils/json_utils.cpp)

add_executable(${PROJECT_NAME}
  src/main.cpp
  src/physics/rigid_body.cpp
)

add_executable(UDPServer
  src/UDPServer.cpp
)

add_executable(UDPClient
  src/UDPClient.cpp
)

target_link_libraries(json_test
  PRIVATE
    nlohmann_json::nlohmann_json
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    proto_lib
    Eigen3::Eigen
    nlohmann_json::nlohmann_json
)

target_link_libraries(UDPServer
  PRIVATE
    proto_lib
    nlohmann_json::nlohmann_json
)

target_link_libraries(UDPClient
  PRIVATE
    proto_lib
    nlohmann_json::nlohmann_json
)

