project(3_HelloCube)
include(${CMAKE_SOURCE_DIR}/cmake/example.cmake)

add_executable(3_HelloCube main.cpp shape_state.cpp)
target_set_windows_application(3_HelloCube) 