add_library(dxvk_d3d11 SHARED IMPORTED)
set_target_properties(dxvk_d3d11 PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/libdxvk_d3d11.so.0"
)

add_library(dxvk_dxgi SHARED IMPORTED)
set_target_properties(dxvk_dxgi PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/libdxvk_dxgi.so.0"
)

# Create interface library that wraps DXVK components
add_library(dxvk INTERFACE)
target_link_libraries(dxvk INTERFACE
    dxvk_d3d11
    dxvk_dxgi
    d3dcompiler
)
target_include_directories(dxvk INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
