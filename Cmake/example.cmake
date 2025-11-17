include(${CMAKE_SOURCE_DIR}/Cmake/common.cmake)

include_directories(${CMAKE_SOURCE_DIR}/Framework ${CMAKE_SOURCE_DIR}/Engine ${CMAKE_SOURCE_DIR}/External)

MACRO(target_set_windows_application target)
    # Copy assets directory to output folder
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Assets
        $<TARGET_FILE_DIR:${target}>/assets
    )

    # Link Engine and DirectX libraries
    target_link_libraries(${target} 
        Engine 
        d3dcompiler  # Shader compiler
        d3d11        # Direct3D 11
        dwmapi       # Desktop Window Manager (for ImGui)
    )
    
    # Console subsystem for debug builds (allows printf/cout)
    if(MINGW)
        set_target_properties(${target} PROPERTIES
            LINK_FLAGS "-Wl,--subsystem,console"
        )
    else()
        # For Clang/MSVC - set console subsystem with main entry point
        set_target_properties(${target} PROPERTIES
            LINK_FLAGS "-Xlinker /SUBSYSTEM:CONSOLE"
        )
    endif()
endmacro()
