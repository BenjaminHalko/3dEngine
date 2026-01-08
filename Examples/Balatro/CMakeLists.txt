project(Balatro)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(Balatro WinMain.cpp GameState.cpp BalatroEffect.cpp)
target_set_windows_application(Balatro)
