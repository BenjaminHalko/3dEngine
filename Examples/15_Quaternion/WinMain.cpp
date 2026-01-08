#include <Engine/Inc/Engine.h>
#include "GameState.h"

int main()
{
    Engine::App& myApp = Engine::MainApp();
    myApp.AddState<GameState>("GameState");

    Engine::AppConfig appConfig;
    appConfig.appName = L"Hello Quaternion";
    appConfig.winWidth = 1280;
    appConfig.winHeight = 720;

    myApp.Run(appConfig);
    return 0;
}
