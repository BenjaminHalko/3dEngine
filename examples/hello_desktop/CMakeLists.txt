project(HelloDesktop)
include(${CMAKE_SOURCE_DIR}/cmake/example.cmake)

add_executable(HelloDesktop WinMain.cpp)
target_set_windows_application(HelloDesktop)
