project(1_HelloDesktop)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(1_HelloDesktop WinMain.cpp)
target_set_windows_application(1_HelloDesktop)
