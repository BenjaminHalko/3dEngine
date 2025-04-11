include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

function(target_compile_library target)
    # First create the target with all source files
    file(GLOB_RECURSE src_files ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
    add_library(${target} ${src_files})

    target_precompile_headers(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/precompiled.h)
endfunction()

# Macro to set up a Windows GUI application (using WinMain)
macro(target_set_windows_application target)
    if(MSVC)
        set_target_properties(
            ${target} PROPERTIES
            LINK_FLAGS_DEBUG "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup"
            LINK_FLAGS_RELEASE "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup"
            LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup"
            LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup"
        )
    endif()
endmacro()
