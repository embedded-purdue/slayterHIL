cmake_minimum_required(VERSION 3.16)
project(flight_sim VERSION 1.0 LANGUAGES CXX)

# --- Compiler Settings ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Useful for clangd/VSCode intellisense

# --- Dependencies ---
find_package(Eigen3 REQUIRED)
find_package(Protobuf REQUIRED)

# --- 1. Protobuf Library (proto_lib) ---
# Generates C++ code from .proto files located in ../shared/proto
set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../shared/proto")
file(GLOB PROTO_FILES "${PROTO_DIR}/*.proto")

add_library(proto_lib STATIC ${PROTO_FILES})

# Link against the official protobuf library
target_link_libraries(proto_lib PUBLIC protobuf::libprotobuf)

# Include the binary directory (where .pb.h files are generated)
target_include_directories(proto_lib PUBLIC 
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Tell CMake to generate the files
protobuf_generate(
    TARGET proto_lib
    LANGUAGE cpp
    IMPORT_DIRS ${PROTO_DIR}
    PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
)

# --- 2. Physics Library (physics_lib) ---
# Compiles all physics logic into a static library
add_library(physics_lib STATIC
    src/physics/drone.cpp
    src/physics/PIDcalculator.cpp
    src/physics/positionController.cpp
    src/physics/rigid_body.cpp
    src/physics/velocityController.cpp
)

# Links Eigen and allows users of this lib to see 'include/'
target_link_libraries(physics_lib PUBLIC Eigen3::Eigen)
target_include_directories(physics_lib PUBLIC include)

# --- 3. Hardware Abstraction Layer (hal_lib) ---
# Compiles the SPI driver implementation
add_library(hal_lib STATIC
    src/spi/spi_rpi.cpp # or spi_linux.cpp
)

# Allows users of hal_lib to see 'include/' (for spi_new_interface.h)
target_include_directories(hal_lib PUBLIC include)

# --- 4. Main Executable (flight_sim) ---
add_executable(flight_sim 
    src/main.cpp
)

# Link all the libraries together
target_link_libraries(flight_sim PRIVATE
    physics_lib
    hal_lib
    proto_lib
    Eigen3::Eigen
)

# --- 5. Tests ---
# Build the SPI test if the file exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/spi_new_test.cpp")
    add_executable(spi_test tests/spi_new_test.cpp)
    target_link_libraries(spi_test PRIVATE hal_lib proto_lib)
endif()
