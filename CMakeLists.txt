cmake_minimum_required(VERSION 3.16)
project(3dEngine)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(Cmake/common.cmake)

add_subdirectory(External)
add_subdirectory(Framework)
add_subdirectory(Engine)
add_subdirectory(Tools)
add_subdirectory(Examples)
