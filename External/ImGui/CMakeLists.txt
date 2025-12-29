project(ImGui)

add_library(ImGui STATIC
    imgui.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    backends/imgui_impl_glfw.cpp
    backends/imgui_impl_dx11.cpp
)

target_include_directories(ImGui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
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
        ${CMAKE_SOURCE_DIR}/External/dxmt/include/directx
        ${CMAKE_SOURCE_DIR}/External/dxmt/include/windows
    )
else()
    # Linux - use dxvk headers and link X11
    target_include_directories(ImGui PUBLIC
        ${CMAKE_SOURCE_DIR}/External/dxvk/include
    )
    find_package(X11 REQUIRED)
    target_link_libraries(ImGui PUBLIC ${X11_LIBRARIES})
endif()
