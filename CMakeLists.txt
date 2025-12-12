cmake_minimum_required(VERSION 3.16)
project(3dEngine)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Compile Commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set common output directory for all targets
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(Cmake/common.cmake)

# Copy assets to output directory (once)
file(COPY ${CMAKE_SOURCE_DIR}/Assets DESTINATION ${CMAKE_BINARY_DIR}/bin)

# Copy platform-specific shared libraries to output directory
if(UNIX AND NOT APPLE)
    file(GLOB DXVK_LIBS ${CMAKE_SOURCE_DIR}/External/dxvk/lib/*.so*)
    file(COPY ${DXVK_LIBS} DESTINATION ${CMAKE_BINARY_DIR}/bin)
endif()

add_subdirectory(External)
add_subdirectory(Framework)
add_subdirectory(Engine)
add_subdirectory(Tools)
add_subdirectory(Examples)
