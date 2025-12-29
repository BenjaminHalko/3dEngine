# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# DirectX11 compatibility
if (NOT WIN32)
    add_subdirectory(vkd3d)

    if (APPLE)
        add_subdirectory(dxmt)
    else()
        add_subdirectory(dxvk)
    endif()
endif()

# Assimp
add_subdirectory(assimp)

# Configure ImGui
project(ImGui)
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ImGui)

add_library(ImGui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)

target_include_directories(ImGui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(ImGui PUBLIC glfw)

if(WIN32)
    target_link_libraries(ImGui PUBLIC
        d3d11
        d3dcompiler
        dwmapi
    )
elseif(APPLE)
    find_library(METAL_LIBRARY Metal)
    find_library(QUARTZCORE_LIBRARY QuartzCore)
    target_link_libraries(ImGui PUBLIC
        ${METAL_LIBRARY}
        ${QUARTZCORE_LIBRARY}
    )
    target_include_directories(ImGui PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/dxmt/include/directx
        ${CMAKE_CURRENT_SOURCE_DIR}/dxmt/include/windows
    )
else()
    # Linux - use dxvk headers and link X11
    target_include_directories(ImGui PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/dxvk/include
    )
    find_package(X11 REQUIRED)
    target_link_libraries(ImGui PUBLIC ${X11_LIBRARIES})
endif()
