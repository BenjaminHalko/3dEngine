#include <Engine/Inc/Engine.h>
#include "GameState.h"
#include "SelfTest.h"

#include <cstring>

int main(int argc, char** argv)
{
    // Headless self-test mode: if "--selftest" is passed, run every
    // deterministic per-subsystem self-test WITHOUT creating the App/window,
    // GraphicsSystem or GameWorld, then return its exit code (0 = all pass,
    // non-zero = a failure). This is the CI/QA path -- no human, no render.
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] != nullptr && std::strcmp(argv[i], "--selftest") == 0)
        {
            return Engine::CriticalCore::RunSelfTests();
        }
    }

    Engine::App& myApp = Engine::MainApp();
    myApp.AddState<GameState>("Game");

    Engine::AppConfig appConfig;
    appConfig.appName = L"Critical Core 2";
    appConfig.winWidth = 768;
    appConfig.winHeight = 672;
    appConfig.maxVertexCount = 100000;

    myApp.Run(appConfig);
    return 0;
}
