project(9_HelloLighting)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(9_HelloLighting WinMain.cpp GameState.cpp)
target_set_windows_application(9_HelloLighting)
