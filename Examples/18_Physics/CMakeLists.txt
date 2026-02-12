project(18_Physics)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(18_Physics WinMain.cpp GameState.cpp)
target_set_windows_application(18_Physics)
