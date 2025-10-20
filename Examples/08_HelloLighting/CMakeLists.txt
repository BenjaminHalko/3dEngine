project(08_HelloLighting)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(08_HelloLighting WinMain.cpp GameState.cpp)
target_set_windows_application(08_HelloLighting)
