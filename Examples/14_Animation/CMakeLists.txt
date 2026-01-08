project(14_Animation)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(14_Animation WinMain.cpp GameState.cpp)
target_set_windows_application(14_Animation)
