project(20_EventsAudio)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(20_EventsAudio WinMain.cpp GameState.cpp)
target_set_windows_application(20_EventsAudio)
