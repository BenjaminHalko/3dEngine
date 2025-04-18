include(${CMAKE_SOURCE_DIR}/cmake/common.cmake)

include_directories(${CMAKE_SOURCE_DIR}/framework ${CMAKE_SOURCE_DIR}/engine ${CMAKE_SOURCE_DIR}/external)

MACRO(target_set_windows_application target)
    target_link_libraries(${target} Engine)
    set_target_properties(
        ${target} PROPERTIES
        LINK_FLAGS_DEBUG "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup"
        LINK_FLAGS_RELEASE "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup"
        LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup"
        LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup"
    )
endmacro()
