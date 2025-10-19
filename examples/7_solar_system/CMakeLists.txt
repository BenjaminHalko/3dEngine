project(8_SolarSystem)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(8_SolarSystem WinMain.cpp GameState.cpp)
target_set_windows_application(8_SolarSystem)
