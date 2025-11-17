project(11_HelloPostProcessing)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(11_HelloPostProcessing WinMain.cpp GameState.cpp)
target_set_windows_application(11_HelloPostProcessing)


