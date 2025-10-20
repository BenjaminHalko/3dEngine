project(02_HelloShapes)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(02_HelloShapes WinMain.cpp ShapeState.cpp)
target_set_windows_application(02_HelloShapes)
