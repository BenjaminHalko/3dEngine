function(target_compile_library target)
    # First create the target with all source files
    file(GLOB_RECURSE src_files ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
    add_library(${target} ${src_files})

    # Add the src directory to include paths for precompiled headers
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)
endfunction()
