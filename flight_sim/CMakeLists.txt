cmake_minimum_required(VERSION 3.22)
project(flight_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Eigen3 3.4 REQUIRED NO_MODULE)
find_package(Protobuf REQUIRED CONFIG)

set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../shared/proto)

add_executable(${PROJECT_NAME} src/main.cpp)

file(GLOB PROTO_FILES "${PROTO_DIR}/*.proto")

protobuf_generate(
    TARGET ${PROJECT_NAME}
    LANGUAGE cpp
    PROTOS ${PROTO_FILES}
    IMPORT_DIRS ${PROTO_DIR}
    PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${PROTO_DIR}
    ${CMAKE_SOURCE_DIR}/lib
)

target_link_libraries(${PROJECT_NAME} PRIVATE Eigen3::Eigen protobuf::libprotobuf)

add_executable(UDPServer src/UDPServer.cpp)
add_executable(UDPClient src/UDPClient.cpp)

