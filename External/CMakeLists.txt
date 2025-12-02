# Add GLFW for cross-platform windowing (used on all platforms)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# Configure ImGui with GLFW backend (universal)
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

# Link GLFW and graphics libraries
target_link_libraries(ImGui PUBLIC glfw)

if(WIN32)
    target_link_libraries(ImGui PUBLIC 
        d3d11 
        d3dcompiler
        dwmapi
    )
elseif(APPLE)
    # On macOS, we'll link DXMT libraries
    find_library(METAL_LIBRARY Metal)
    find_library(QUARTZCORE_LIBRARY QuartzCore)
    target_link_libraries(ImGui PUBLIC 
        ${METAL_LIBRARY}
        ${QUARTZCORE_LIBRARY}
    )
endif()

set_target_properties(ImGui PROPERTIES FOLDER "External")

# Configure DXMT for macOS
if(APPLE)
    set(DXMT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dxmt)

    # Build D3DCompiler implementation
    add_library(d3dcompiler STATIC
        ${DXMT_DIR}/d3dcompiler.cpp
    )
    target_include_directories(d3dcompiler PRIVATE
        ${DXMT_DIR}/include/directx
        ${DXMT_DIR}/include/windows
    )

    # Import pre-built native macdrv bridge for GLFW-Metal integration
    add_library(native_macdrv_bridge SHARED IMPORTED)
    set_target_properties(native_macdrv_bridge PROPERTIES
        IMPORTED_LOCATION "${DXMT_DIR}/libnative_macdrv_bridge.dylib"
    )

    # Import pre-built DXMT shared libraries as proper IMPORTED targets
    add_library(dxmt_d3d11 SHARED IMPORTED)
    set_target_properties(dxmt_d3d11 PROPERTIES IMPORTED_LOCATION "${DXMT_DIR}/d3d11.dylib")

    add_library(dxmt_dxgi SHARED IMPORTED)
    set_target_properties(dxmt_dxgi PROPERTIES IMPORTED_LOCATION "${DXMT_DIR}/dxgi.dylib")

    add_library(dxmt_winemetal SHARED IMPORTED)
    set_target_properties(dxmt_winemetal PROPERTIES IMPORTED_LOCATION "${DXMT_DIR}/winemetal.dylib")

    # Import static libraries
    add_library(dxmt_core STATIC IMPORTED)
    set_target_properties(dxmt_core PROPERTIES IMPORTED_LOCATION "${DXMT_DIR}/libdxmt.a")

    add_library(dxmt_airconv STATIC IMPORTED)
    set_target_properties(dxmt_airconv PROPERTIES IMPORTED_LOCATION "${DXMT_DIR}/libairconv.a")

    add_library(dxmt_util STATIC IMPORTED)
    set_target_properties(dxmt_util PROPERTIES IMPORTED_LOCATION "${DXMT_DIR}/libutil.a")

    # Create interface library that wraps all DXMT components
    add_library(dxmt INTERFACE)
    add_dependencies(dxmt native_macdrv_bridge d3dcompiler)
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

    # Also add ImGui DXMT include directories
    target_include_directories(ImGui PUBLIC
        ${DXMT_DIR}/include/directx
        ${DXMT_DIR}/include/windows
    )
endif()

# Configure Assimp
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
add_subdirectory(assimp)
