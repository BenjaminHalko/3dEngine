project(09_CellShader)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(09_CellShader WinMain.cpp GameState.cpp)
target_set_windows_application(09_CellShader)
