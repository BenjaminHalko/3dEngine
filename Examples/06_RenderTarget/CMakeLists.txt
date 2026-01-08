project(06_RenderTarget)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(06_RenderTarget WinMain.cpp GameState.cpp)
target_set_windows_application(06_RenderTarget)
