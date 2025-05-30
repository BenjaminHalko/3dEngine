project(Input)
include(${CMAKE_SOURCE_DIR}/cmake/framework_lib.cmake)

target_compile_library(Input)

target_link_libraries(Input PRIVATE Core)
