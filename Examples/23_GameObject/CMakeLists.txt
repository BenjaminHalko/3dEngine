project(23_GameObject)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(23_GameObject WinMain.cpp GameState.cpp)
target_set_windows_application(23_GameObject)
