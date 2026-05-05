project(24_GameObjectFactory)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(24_GameObjectFactory WinMain.cpp GameState.cpp)
target_set_windows_application(24_GameObjectFactory)
