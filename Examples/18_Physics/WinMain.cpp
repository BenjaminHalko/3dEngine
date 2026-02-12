#include <Engine/Inc/Engine.h>
#include "GameState.h"

int main()
{
    Engine::App& myApp = Engine::MainApp();
    myApp.AddState<GameState>("GameState");

    Engine::AppConfig appConfig;
    appConfig.appName = L"Physics Demo";
    appConfig.winWidth = 1920;
    appConfig.winHeight = 1080;
    appConfig.maxVertexCount = 100000;

    myApp.Run(appConfig);
    return 0;
}
