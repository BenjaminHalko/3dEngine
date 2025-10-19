project(Graphics)
include(${CMAKE_SOURCE_DIR}/Cmake/framework_lib.cmake)

target_compile_library(Graphics)

target_link_libraries(Graphics PRIVATE Core Math ImGui)
target_link_directories(Graphics PRIVATE ${CMAKE_SOURCE_DIR}/external/DirectXTK)
