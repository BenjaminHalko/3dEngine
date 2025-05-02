project(1_HelloDesktop)
include(${CMAKE_SOURCE_DIR}/cmake/example.cmake)

add_executable(1_HelloDesktop main.cpp)
target_set_windows_application(1_HelloDesktop)
