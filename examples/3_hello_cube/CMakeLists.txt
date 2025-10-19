project(3_HelloCube)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(3_HelloCube WinMain.cpp ShapeState.cpp)
target_set_windows_application(3_HelloCube) 