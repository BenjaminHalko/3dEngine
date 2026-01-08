project(07_SolarSystem)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(07_SolarSystem WinMain.cpp GameState.cpp)
target_set_windows_application(07_SolarSystem)
