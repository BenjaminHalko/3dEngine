project(4_HelloMeshBuilder)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(4_HelloMeshBuilder WinMain.cpp ShapeState.cpp)
target_set_windows_application(4_HelloMeshBuilder)