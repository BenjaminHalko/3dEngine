# Metal bridge for native DXMT (provides macdrv symbols)
add_library(dxmt_bridge STATIC src/MetalBridge.mm)
target_compile_options(dxmt_bridge PRIVATE -fobjc-arc)
target_link_libraries(dxmt_bridge PRIVATE
    "-framework Metal"
    "-framework QuartzCore"
    "-framework Cocoa"
)
# Export symbols to dynamic symbol table so dlsym can find them
set_target_properties(dxmt_bridge PROPERTIES
    INTERFACE_LINK_OPTIONS "-Wl,-exported_symbol,_get_win_data;-Wl,-exported_symbol,_release_win_data;-Wl,-exported_symbol,_macdrv_view_create_metal_view;-Wl,-exported_symbol,_macdrv_view_get_metal_layer;-Wl,-exported_symbol,_macdrv_view_release_metal_view"
)

# Import pre-built DXMT shared libraries
add_library(dxmt_d3d11 SHARED IMPORTED)
set_target_properties(dxmt_d3d11 PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/d3d11.dylib"
)

add_library(dxmt_dxgi SHARED IMPORTED)
set_target_properties(dxmt_dxgi PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/dxgi.dylib"
)

add_library(dxmt_winemetal SHARED IMPORTED)
set_target_properties(dxmt_winemetal PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/winemetal.dylib"
)

# Import static libraries
add_library(dxmt_core STATIC IMPORTED)
set_target_properties(dxmt_core PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/libdxmt.a"
)

add_library(dxmt_airconv STATIC IMPORTED)
set_target_properties(dxmt_airconv PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/libairconv.a"
)

add_library(dxmt_util STATIC IMPORTED)
set_target_properties(dxmt_util PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/libutil.a"
)

# Create interface library that wraps all DXMT components
add_library(dxmt INTERFACE)
target_link_libraries(dxmt INTERFACE
    dxmt_bridge
    dxmt_d3d11
    dxmt_dxgi
    dxmt_winemetal
    dxmt_core
    dxmt_airconv
    dxmt_util
    d3dcompiler
)

target_include_directories(dxmt INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/directx
    ${CMAKE_CURRENT_SOURCE_DIR}/include/windows
)
