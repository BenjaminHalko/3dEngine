project(26_CriticalCore)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)
add_executable(26_CriticalCore WinMain.cpp GameState.cpp)
target_set_windows_application(26_CriticalCore)
