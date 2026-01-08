project(03_Cube)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(03_Cube WinMain.cpp ShapeState.cpp)
target_set_windows_application(03_Cube)
