project(11_PostProcessing)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(11_PostProcessing WinMain.cpp GameState.cpp)
target_set_windows_application(11_PostProcessing)
