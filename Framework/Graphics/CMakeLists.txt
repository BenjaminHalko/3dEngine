project(Graphics)
include(${CMAKE_SOURCE_DIR}/Cmake/framework_lib.cmake)

target_compile_library(Graphics)

# Exclude X11Window.cpp from precompiled headers (it has X11/Window name conflict)
if(UNIX AND NOT APPLE)
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/X11Window.cpp
        PROPERTIES SKIP_PRECOMPILE_HEADERS ON
    )
endif()

target_link_libraries(Graphics PUBLIC ImGui glfw PRIVATE Core Math)

if(WIN32)
    # Windows: Use native DirectX (linked at example level)
elseif(APPLE)
    # macOS: Link DXMT and required frameworks
    target_link_libraries(Graphics PUBLIC dxmt)
    find_library(METAL_LIBRARY Metal)
    find_library(QUARTZCORE_LIBRARY QuartzCore)
    find_library(FOUNDATION_LIBRARY Foundation)
    find_library(APPKIT_LIBRARY AppKit)
    target_link_libraries(Graphics PUBLIC
        ${METAL_LIBRARY}
        ${QUARTZCORE_LIBRARY}
        ${FOUNDATION_LIBRARY}
        ${APPKIT_LIBRARY}
    )
else()
    # Linux: Link DXVK-native
    target_link_libraries(Graphics PUBLIC dxvk)
endif()
