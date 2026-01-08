project(10_Model)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(10_Model WinMain.cpp GameState.cpp)
target_set_windows_application(10_Model)
