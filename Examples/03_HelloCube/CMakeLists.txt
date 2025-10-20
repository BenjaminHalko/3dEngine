project(03_HelloCube)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(03_HelloCube WinMain.cpp ShapeState.cpp)
target_set_windows_application(03_HelloCube) 