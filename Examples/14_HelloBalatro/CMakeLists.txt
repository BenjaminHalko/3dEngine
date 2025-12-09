project(14_HelloBalatro)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(14_HelloBalatro WinMain.cpp GameState.cpp BalatroEffect.cpp)
target_set_windows_application(14_HelloBalatro)
