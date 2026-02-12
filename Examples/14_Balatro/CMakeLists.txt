project(14_Balatro)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(14_Balatro WinMain.cpp GameState.cpp BalatroEffect.cpp)
target_set_windows_application(14_Balatro)
