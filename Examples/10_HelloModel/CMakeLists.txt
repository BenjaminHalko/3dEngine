project(10_HelloModel)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(10_HelloModel WinMain.cpp GameState.cpp)
target_set_windows_application(10_HelloModel)

