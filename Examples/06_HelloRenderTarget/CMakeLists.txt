project(06_HelloRenderTarget)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(06_HelloRenderTarget WinMain.cpp GameState.cpp)
target_set_windows_application(06_HelloRenderTarget)