project(Engine)
include(${CMAKE_SOURCE_DIR}/cmake/engine_lib.cmake)

target_compile_library(Engine)
target_link_libraries(Engine Core Graphics Input Math)
