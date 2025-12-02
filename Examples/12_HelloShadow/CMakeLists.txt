project(12_HelloShadow)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(12_HelloShadow WinMain.cpp GameState.cpp)
target_set_windows_application(12_HelloShadow)
