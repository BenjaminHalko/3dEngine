project(04_MeshBuilder)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(04_MeshBuilder WinMain.cpp ShapeState.cpp)
target_set_windows_application(04_MeshBuilder)
