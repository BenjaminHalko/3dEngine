project(05_Debug)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(05_Debug WinMain.cpp GameState.cpp)
target_set_windows_application(05_Debug)
