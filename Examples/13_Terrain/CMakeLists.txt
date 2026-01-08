project(13_Terrain)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(13_Terrain WinMain.cpp GameState.cpp)
target_set_windows_application(13_Terrain)
