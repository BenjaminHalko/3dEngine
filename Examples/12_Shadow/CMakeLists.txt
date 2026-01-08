project(12_Shadow)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(12_Shadow WinMain.cpp GameState.cpp)
target_set_windows_application(12_Shadow)
