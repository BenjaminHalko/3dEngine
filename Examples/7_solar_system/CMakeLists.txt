project(7_SolarSystem)
include(${CMAKE_SOURCE_DIR}/cmake/example.cmake)

add_executable(7_SolarSystem WinMain.cpp GameState.cpp)
target_set_windows_application(7_SolarSystem)
