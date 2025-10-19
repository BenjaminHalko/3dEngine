project(8_HelloLighting)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(8_HelloLighting WinMain.cpp GameState.cpp)
target_set_windows_application(8_HelloLighting)
