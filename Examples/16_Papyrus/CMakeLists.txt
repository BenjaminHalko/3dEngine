project(16_Papyrus)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(16_Papyrus WinMain.cpp GameState.cpp)
target_set_windows_application(16_Papyrus)
