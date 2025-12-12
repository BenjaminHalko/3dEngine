# Add GLFW for cross-platform windowing (used on all platforms)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# Configure d3dcompiler compatibility layer using vendored vkd3d-utils (non-Windows only)
if(NOT WIN32)
    set(VKD3D_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vkd3d)

    add_library(vkd3d-common STATIC IMPORTED)
    set_target_properties(vkd3d-common PROPERTIES
        IMPORTED_LOCATION "${VKD3D_DIR}/lib/libvkd3d-common.a"
    )

    add_library(vkd3d STATIC IMPORTED)
    set_target_properties(vkd3d PROPERTIES
        IMPORTED_LOCATION "${VKD3D_DIR}/lib/libvkd3d.a"
    )

    add_library(vkd3d-shader STATIC IMPORTED)
    set_target_properties(vkd3d-shader PROPERTIES
        IMPORTED_LOCATION "${VKD3D_DIR}/lib/libvkd3d-shader.a"
    )

    add_library(vkd3d-utils STATIC IMPORTED)
    set_target_properties(vkd3d-utils PROPERTIES
        IMPORTED_LOCATION "${VKD3D_DIR}/lib/libvkd3d-utils.a"
    )

    # d3dcompiler interface wraps vkd3d-utils which provides D3DCompile
    add_library(d3dcompiler INTERFACE)
    target_link_libraries(d3dcompiler INTERFACE vkd3d-utils vkd3d-shader vkd3d vkd3d-common)
    target_include_directories(d3dcompiler INTERFACE ${VKD3D_DIR}/include)
endif()

# Configure ImGui with GLFW backend
project(ImGui)
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ImGui)

add_library(ImGui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
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

set_target_properties(ImGui PROPERTIES FOLDER "External")

# Configure DXMT for macOS
if(APPLE)
    set(DXMT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dxmt)

    # Import pre-built native macdrv bridge for GLFW-Metal integration
    add_library(native_macdrv_bridge SHARED IMPORTED)
    set_target_properties(native_macdrv_bridge PROPERTIES
        IMPORTED_LOCATION "${DXMT_DIR}/lib/libnative_macdrv_bridge.dylib"
    )

    # Import pre-built DXMT shared libraries
    add_library(dxmt_d3d11 SHARED IMPORTED)
    set_target_properties(dxmt_d3d11 PROPERTIES IMPORTED_LOCATION "${DXMT_DIR}/lib/d3d11.dylib")

    add_library(dxmt_dxgi SHARED IMPORTED)
    set_target_properties(dxmt_dxgi PROPERTIES IMPORTED_LOCATION "${DXMT_DIR}/lib/dxgi.dylib")

    add_library(dxmt_winemetal SHARED IMPORTED)
    set_target_properties(dxmt_winemetal PROPERTIES IMPORTED_LOCATION "${DXMT_DIR}/lib/winemetal.dylib")

    # Import static libraries
    add_library(dxmt_core STATIC IMPORTED)
    set_target_properties(dxmt_core PROPERTIES IMPORTED_LOCATION "${DXMT_DIR}/lib/libdxmt.a")

    add_library(dxmt_airconv STATIC IMPORTED)
    set_target_properties(dxmt_airconv PROPERTIES IMPORTED_LOCATION "${DXMT_DIR}/lib/libairconv.a")

    add_library(dxmt_util STATIC IMPORTED)
    set_target_properties(dxmt_util PROPERTIES IMPORTED_LOCATION "${DXMT_DIR}/libutil.a")

    # Create interface library that wraps all DXMT components
    add_library(dxmt INTERFACE)
    target_link_libraries(dxmt INTERFACE
        native_macdrv_bridge
        dxmt_d3d11
        dxmt_dxgi
        dxmt_winemetal
        dxmt_core
        dxmt_airconv
        dxmt_util
        d3dcompiler
    )
    target_include_directories(dxmt INTERFACE
        ${DXMT_DIR}/include/directx
        ${DXMT_DIR}/include/windows
    )
endif()

# Configure DXVK-native for Linux
if(UNIX AND NOT APPLE)
    set(DXVK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dxvk)

    # Import pre-built DXVK shared libraries
    add_library(dxvk_d3d11 SHARED IMPORTED)
    set_target_properties(dxvk_d3d11 PROPERTIES
        IMPORTED_LOCATION "${DXVK_DIR}/lib/libdxvk_d3d11.so.0"
        IMPORTED_SONAME "libdxvk_d3d11.so.0"
    )

    add_library(dxvk_dxgi SHARED IMPORTED)
    set_target_properties(dxvk_dxgi PROPERTIES
        IMPORTED_LOCATION "${DXVK_DIR}/lib/libdxvk_dxgi.so.0"
        IMPORTED_SONAME "libdxvk_dxgi.so.0"
    )

    # Create interface library that wraps DXVK components
    add_library(dxvk INTERFACE)
    target_link_libraries(dxvk INTERFACE
        dxvk_d3d11
        dxvk_dxgi
        d3dcompiler
    )
    target_include_directories(dxvk INTERFACE
        ${DXVK_DIR}/include
    )
endif()

# Configure vendored Assimp
set(ASSIMP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assimp)

add_library(assimp SHARED IMPORTED GLOBAL)
target_include_directories(assimp INTERFACE ${ASSIMP_DIR}/include)

if(WIN32)
    set_target_properties(assimp PROPERTIES
        IMPORTED_LOCATION "${ASSIMP_DIR}/lib/assimp.lib"
        IMPORTED_IMPLIB "${ASSIMP_DIR}/lib/assimp.lib"
    )
elseif(APPLE)
    set_target_properties(assimp PROPERTIES
        IMPORTED_LOCATION "${ASSIMP_DIR}/lib/libassimp.dylib"
    )
else()
    set_target_properties(assimp PROPERTIES
        IMPORTED_LOCATION "${ASSIMP_DIR}/lib/libassimp.so"
    )
endif()
