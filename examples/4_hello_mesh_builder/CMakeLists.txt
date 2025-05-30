project(4_HelloMeshBuilder)
include(${CMAKE_SOURCE_DIR}/cmake/example.cmake)

add_executable(4_HelloMeshBuilder main.cpp shape_state.cpp)
target_set_windows_application(4_HelloMeshBuilder)