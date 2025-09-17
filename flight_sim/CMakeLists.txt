cmake_minimum_required(VERSION 3.16)

project(flight_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Eigen3 3.4 REQUIRED NO_MODULE)

add_executable(${PROJECT_NAME} src/main.cpp)


target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/lib)

target_link_libraries(${PROJECT_NAME} PRIVATE Eigen3::Eigen)