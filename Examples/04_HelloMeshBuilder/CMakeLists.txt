project(04_HelloMeshBuilder)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(04_HelloMeshBuilder WinMain.cpp ShapeState.cpp)
target_set_windows_application(04_HelloMeshBuilder)