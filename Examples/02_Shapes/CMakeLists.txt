project(02_Shapes)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(02_Shapes WinMain.cpp ShapeState.cpp)
target_set_windows_application(02_Shapes)
