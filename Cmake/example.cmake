include_directories(${CMAKE_SOURCE_DIR}/Framework ${CMAKE_SOURCE_DIR}/Engine ${CMAKE_SOURCE_DIR}/External)

MACRO(target_set_windows_application target)
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
        # Linux: Link Engine (which includes DXVK)
        target_link_libraries(${target} Engine)
    endif()
endmacro()
