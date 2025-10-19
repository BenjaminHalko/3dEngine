include(${CMAKE_SOURCE_DIR}/Cmake/common.cmake)

include_directories(${CMAKE_SOURCE_DIR}/Framework ${CMAKE_SOURCE_DIR}/Engine ${CMAKE_SOURCE_DIR}/External)

MACRO(target_set_windows_application target)
    # copy assets directory to output folder
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Assets
        $<TARGET_FILE_DIR:${target}>/assets
    )

    target_link_libraries(${target} Engine DirectXTK)
    if(WIN32)
        # Use console subsystem with WinMain entry point for debug builds
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set_target_properties(${target} PROPERTIES
                LINK_FLAGS "-Xlinker /SUBSYSTEM:CONSOLE -Xlinker /ENTRY:WinMainCRTStartup"
            )
        else()
            set_target_properties(${target} PROPERTIES WIN32_EXECUTABLE TRUE)
        endif()
        target_link_libraries(${target} d3dcompiler d3d11)
    endif()
endmacro()
