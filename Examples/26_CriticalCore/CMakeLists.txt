project(26_CriticalCore)
include(${CMAKE_SOURCE_DIR}/Cmake/example.cmake)
add_executable(26_CriticalCore
    WinMain.cpp
    SelfTest.cpp
    GameState.cpp
    AnimCurve.cpp
    GmHelpers.cpp
    Render2D.cpp
    GameClock.cpp
    CustomRegistration.cpp
    StubComponent.cpp
    RoundConfig.cpp
    Collision.cpp
    RenderTarget2D.cpp
    CameraShakeService.cpp
    CriticalCore2DRenderService.cpp
    Render2DComponent.cpp
    MusicController.cpp
    Leaderboard.cpp
    HttpFetch.cpp
    LeaderboardFetcher.cpp
    EntityComponent.cpp
    CoreComponent.cpp
    WallComponent.cpp
    TrailComponent.cpp
    SparkleComponent.cpp
    BackgroundComponent.cpp
    ScoreComponent.cpp
    GuiComponent.cpp
    MenuComponent.cpp
    PlayerComponent.cpp
    BubbleComponent.cpp
    FireballComponent.cpp
    SpikeComponent.cpp
    GameFlow.cpp)
target_set_windows_application(26_CriticalCore)

# OpenMP for the LeaderboardFetcher background HTTP dispatch. Apple's stock
# Clang ships without OpenMP runtime so on macOS we hint at the Homebrew libomp
# install (`brew install libomp`) before re-running find_package; on Linux/
# Windows / Homebrew LLVM Clang the first find_package call succeeds directly.
find_package(OpenMP)
if(NOT OpenMP_CXX_FOUND AND APPLE)
    foreach(_libomp_prefix "/opt/homebrew/opt/libomp" "/usr/local/opt/libomp")
        if(EXISTS "${_libomp_prefix}/include/omp.h")
            set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp -I${_libomp_prefix}/include")
            set(OpenMP_CXX_LIB_NAMES "omp")
            set(OpenMP_omp_LIBRARY "${_libomp_prefix}/lib/libomp.dylib")
            break()
        endif()
    endforeach()
    find_package(OpenMP)
endif()
if(OpenMP_CXX_FOUND)
    target_link_libraries(26_CriticalCore OpenMP::OpenMP_CXX)
else()
    message(WARNING "OpenMP not found - LeaderboardFetcher will fall back to a sequential fetch.")
endif()
