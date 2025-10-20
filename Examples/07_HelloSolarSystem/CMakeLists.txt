project(07_HelloSolarSystem)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(07_HelloSolarSystem WinMain.cpp GameState.cpp)
target_set_windows_application(07_HelloSolarSystem)
