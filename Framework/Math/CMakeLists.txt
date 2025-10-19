project(Math)
include(${CMAKE_SOURCE_DIR}/Cmake/framework_lib.cmake)

target_compile_library(Math)

target_link_libraries(Math PRIVATE Core)
