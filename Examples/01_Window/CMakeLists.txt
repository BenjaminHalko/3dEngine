project(01_Window)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(01_Window WinMain.cpp)
target_set_windows_application(01_Window)
