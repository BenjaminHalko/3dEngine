project(19_ParticleSystem)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(19_ParticleSystem WinMain.cpp GameState.cpp)
target_set_windows_application(19_ParticleSystem)
