project(6_HelloRenderTarget)
include(${CMAKE_SOURCE_DIR}/cmake/example.cmake)

add_executable(6_HelloRenderTarget WinMain.cpp GameState.cpp)
target_set_windows_application(6_HelloRenderTarget)