project(21_AnimatedPlane)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(21_AnimatedPlane WinMain.cpp GameState.cpp)
target_set_windows_application(21_AnimatedPlane)
