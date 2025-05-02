project(Graphics)
include(${CMAKE_SOURCE_DIR}/cmake/framework_lib.cmake)

target_compile_library(Graphics)

target_link_libraries(Graphics PRIVATE Core Math)
target_link_directories(Graphics PRIVATE ${CMAKE_SOURCE_DIR}/external/DirectXTK)
