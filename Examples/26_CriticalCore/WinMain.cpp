#include <Engine/Inc/Engine.h>
#include "GameState.h"

int main()
{
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
