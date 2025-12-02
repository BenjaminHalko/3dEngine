include(${CMAKE_SOURCE_DIR}/Cmake/common.cmake)
include(${CMAKE_SOURCE_DIR}/Cmake/compileshaders.cmake)

include_directories(${CMAKE_SOURCE_DIR}/Framework ${CMAKE_SOURCE_DIR}/Engine ${CMAKE_SOURCE_DIR}/External)

MACRO(target_set_windows_application target)
    # Copy assets directory to output folder
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Assets
        $<TARGET_FILE_DIR:${target}>/assets
    )

    # Compile shaders to .cso files
    if(FXC_EXECUTABLE)
        # Compile to build directory, then copy with other assets
        set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/Assets/Shaders")
        compile_all_shaders(
            "${CMAKE_SOURCE_DIR}/Assets/Shaders"
            "${SHADER_OUTPUT_DIR}"
            ${target}
        )
        add_dependencies(${target} ${target}_shaders)
    endif()

    if(WIN32)
        # Windows: Link DirectX libraries
        target_link_libraries(${target}
            Engine
            d3dcompiler  # Shader compiler
            d3d11        # Direct3D 11
            dwmapi       # Desktop Window Manager (for ImGui)
        )

        # For Clang/MSVC - set console subsystem with main entry point
        set_target_properties(${target} PROPERTIES
            LINK_FLAGS "-Xlinker /SUBSYSTEM:CONSOLE"
        )
    elseif(APPLE)
        # macOS: Link Engine (which includes DXMT via shared libraries)
        target_link_libraries(${target} Engine)
    else()
        # Linux: Add appropriate libraries here
        target_link_libraries(${target} Engine)
    endif()
endmacro()
