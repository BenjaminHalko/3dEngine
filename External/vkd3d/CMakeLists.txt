if(APPLE)
    add_library(vkd3d SHARED IMPORTED)
    add_library(vkd3d-shader SHARED IMPORTED)
    add_library(vkd3d-utils SHARED IMPORTED)

    set_target_properties(vkd3d PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/macos/libvkd3d.1.dylib"
    )
    set_target_properties(vkd3d-shader PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/macos/libvkd3d-shader.1.dylib"
    )
    set_target_properties(vkd3d-utils PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/macos/libvkd3d-utils.1.dylib"
    )
else()
    # Linux uses static libraries
    add_library(vkd3d STATIC IMPORTED)
    add_library(vkd3d-shader STATIC IMPORTED)
    add_library(vkd3d-utils STATIC IMPORTED)

    set_target_properties(vkd3d PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/linux/libvkd3d.a"
    )
    set_target_properties(vkd3d-shader PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/linux/libvkd3d-shader.a"
    )
    set_target_properties(vkd3d-utils PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/linux/libvkd3d-utils.a"
    )
endif()

add_library(d3dcompiler INTERFACE)
target_link_libraries(d3dcompiler INTERFACE vkd3d-utils vkd3d-shader vkd3d)
target_include_directories(d3dcompiler INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
