project(25_GameWorld)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(25_GameWorld
    WinMain.cpp
    GameState.cpp
    CustomDebugDrawComponent.cpp
    CustomDebugDrawService.cpp
)
target_set_windows_application(25_GameWorld)
