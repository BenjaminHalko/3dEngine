project(09_HelloCellShader)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(09_HelloCellShader WinMain.cpp GameState.cpp)
target_set_windows_application(09_HelloCellShader)

