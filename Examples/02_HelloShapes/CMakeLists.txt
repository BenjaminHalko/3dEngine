project(2_HelloShapes)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(2_HelloShapes WinMain.cpp ShapeState.cpp)
target_set_windows_application(2_HelloShapes)
