project(05_HelloDebug)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(05_HelloDebug WinMain.cpp GameState.cpp)
target_set_windows_application(05_HelloDebug)