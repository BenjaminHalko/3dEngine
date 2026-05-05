project(22_Memory)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(22_Memory WinMain.cpp)
target_set_windows_application(22_Memory)
