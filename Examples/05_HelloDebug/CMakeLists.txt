project(5_HelloDebug)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(5_HelloDebug WinMain.cpp GameState.cpp)
target_set_windows_application(5_HelloDebug)