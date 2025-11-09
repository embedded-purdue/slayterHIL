cmake_minimum_required(VERSION 3.22)
project(flight_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Eigen3 3.4 REQUIRED NO_MODULE)
find_package(Protobuf REQUIRED CONFIG)

set(GEN_PROTO_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GEN_PROTO_DIR})

if (TARGET protobuf::protoc)
  set(Protobuf_PROTOC_EXECUTABLE $<TARGET_FILE:protobuf::protoc>)
endif()

set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../shared/proto)
file(GLOB PROTO_FILES "${PROTO_DIR}/*.proto")

add_library(proto_lib STATIC ${PROTO_FILES})

protobuf_generate(
    TARGET proto_lib
    LANGUAGE cpp
    PROTOS ${PROTO_FILES}
    IMPORT_DIRS ${PROTO_DIR}
    PROTOC_OUT_DIR ${GEN_PROTO_DIR}
)

target_link_libraries(proto_lib PUBLIC protobuf::libprotobuf)
target_include_directories(proto_lib PUBLIC ${GEN_PROTO_DIR})

add_executable(${PROJECT_NAME} src/main.cpp)
add_executable(UDPServer src/UDPServer.cpp)
add_executable(UDPClient src/UDPClient.cpp)


target_link_libraries(${PROJECT_NAME} PRIVATE proto_lib Eigen3::Eigen)
target_link_libraries(UDPServer PRIVATE proto_lib)
target_link_libraries(UDPClient PRIVATE proto_lib)
