project(4_HelloMeshBuilder)
include(${CMAKE_SOURCE_DIR}/cmake/example.cmake)

add_executable(4_HelloMeshBuilder WinMain.cpp ShapeState.cpp)
target_set_windows_application(4_HelloMeshBuilder)