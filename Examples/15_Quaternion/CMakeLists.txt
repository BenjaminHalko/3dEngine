project(15_Quaternion)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(15_Quaternion WinMain.cpp GameState.cpp)
target_set_windows_application(15_Quaternion)
