project(08_Lighting)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(08_Lighting WinMain.cpp GameState.cpp)
target_set_windows_application(08_Lighting)
