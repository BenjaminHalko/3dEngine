function(target_compile_library target)
    file(GLOB_RECURSE src_files ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
    add_library(${target} ${src_files})

    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)
endfunction()
