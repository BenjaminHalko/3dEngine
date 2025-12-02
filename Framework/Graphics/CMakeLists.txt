project(Graphics)
include(${CMAKE_SOURCE_DIR}/Cmake/framework_lib.cmake)

target_compile_library(Graphics)

target_link_libraries(Graphics PUBLIC ImGui glfw PRIVATE Core Math)

if(APPLE)
    # Link DXMT interface library and required macOS frameworks
    # Use PUBLIC so these propagate to consumers (Engine, examples)
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
    target_link_directories(Graphics PRIVATE ${CMAKE_SOURCE_DIR}/external/DirectXTK)
endif()
