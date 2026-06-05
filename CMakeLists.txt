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

# Copy assets to output directory once at CONFIGURE time (bootstraps build/bin/Assets
# for the very first build before the CopyAssets target below has run).
file(COPY ${CMAKE_SOURCE_DIR}/Assets DESTINATION ${CMAKE_BINARY_DIR}/bin)

# Refresh assets on EVERY build, not just at configure time. The file(COPY ...)
# above only runs when CMake (re)configures, so edits to templates/shaders/etc.
# would otherwise leave build/bin/Assets STALE until the next reconfigure - which
# repeatedly masked fixes (the example ran old assets). This ALL custom target
# re-copies the whole Assets tree on each `cmake --build`, guaranteeing freshness.
add_custom_target(CopyAssets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/Assets
            ${CMAKE_BINARY_DIR}/bin/Assets
    COMMENT "Refreshing runtime assets in build/bin/Assets"
    VERBATIM)

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

# Make the Critical Core example depend on the asset refresh so building just that
# target (not only the default ALL build) also re-copies assets first. CopyAssets
# is ALL, so any `cmake --build build` refreshes assets for every example anyway.
add_dependencies(26_CriticalCore CopyAssets)
