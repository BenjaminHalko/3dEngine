include(${CMAKE_SOURCE_DIR}/cmake/common.cmake)

include_directories(${CMAKE_SOURCE_DIR}/framework ${CMAKE_SOURCE_DIR}/engine ${CMAKE_SOURCE_DIR}/external)

MACRO(target_set_windows_application target)
    # copy assets directory to output folder
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${target}>/assets
    )

    target_link_libraries(${target} Engine)
    if(MSVC)
        set_target_properties(
            ${target} PROPERTIES
            LINK_FLAGS_DEBUG "/ENTRY:WinMainCRTStartup"
            LINK_FLAGS_RELEASE "/ENTRY:WinMainCRTStartup"
            LINK_FLAGS_RELWITHDEBINFO "/ENTRY:WinMainCRTStartup"
            LINK_FLAGS_MINSIZEREL "/ENTRY:WinMainCRTStartup"
        )
        target_link_libraries(${target} d3dcompiler d3d11)
    endif()
endmacro()
