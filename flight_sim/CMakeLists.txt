cmake_minimum_required(VERSION 3.22)
project(flight_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Eigen3 3.4 REQUIRED NO_MODULE)
find_package(Protobuf REQUIRED CONFIG)

# --- 1. Create a library for your generated Protobuf code ---
# This is a much cleaner way to handle shared protos.
set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../shared/proto)
file(GLOB PROTO_FILES "${PROTO_DIR}/*.proto")

# Create a static library named 'proto_lib' from your .proto files
add_library(proto_lib STATIC ${PROTO_FILES})

# Generate C++ code FOR the 'proto_lib' target.
# This attaches the generated .pb.cc files to the library.
protobuf_generate(
    TARGET proto_lib
    LANGUAGE cpp
    PROTOS ${PROTO_FILES}
    IMPORT_DIRS ${PROTO_DIR}
    PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
)

# Publicly link proto_lib against the main protobuf library.
# This ensures that any executable linking against proto_lib also links against protobuf.
target_link_libraries(proto_lib PUBLIC protobuf::libprotobuf)


# --- 2. Define your executables ---
add_executable(${PROJECT_NAME} src/main.cpp)
add_executable(UDPServer src/UDPServer.cpp)
add_executable(UDPClient src/UDPClient.cpp)


# --- 3. Link ALL executables that need Protobuf against your new library ---
# By linking to 'proto_lib', they get the generated code, include paths,
# and the main protobuf library dependency all at once.
target_link_libraries(${PROJECT_NAME} PRIVATE proto_lib Eigen3::Eigen)
target_link_libraries(UDPServer PRIVATE proto_lib)
target_link_libraries(UDPClient PRIVATE proto_lib)

# This is no longer needed, as linking to proto_lib handles the include dirs.
# target_include_directories(${PROJECT_NAME}
#   PRIVATE
#     ${PROTO_DIR}
#     ${CMAKE_SOURCE_DIR}/lib
# )