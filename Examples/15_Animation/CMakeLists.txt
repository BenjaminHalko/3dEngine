project(15_Animation)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(15_Animation WinMain.cpp GameState.cpp)
target_set_windows_application(15_Animation)
