project(13_HelloTerrain)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(13_HelloTerrain WinMain.cpp GameState.cpp)
target_set_windows_application(13_HelloTerrain)
