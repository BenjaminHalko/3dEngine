project(01_HelloWindow)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(01_HelloWindow WinMain.cpp)
target_set_windows_application(01_HelloWindow)
