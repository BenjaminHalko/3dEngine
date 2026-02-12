project(17_SkeletonAnimation)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)

add_executable(17_SkeletonAnimation WinMain.cpp GameState.cpp)
target_set_windows_application(17_SkeletonAnimation)
