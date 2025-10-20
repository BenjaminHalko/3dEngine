#include "GameState.h"

using namespace Engine;

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    AppConfig config;
    config.appName = L"Solar System";

    App& myApp = MainApp();

    myApp.AddState<GameState>("GameState");

    myApp.Run(config);

    return 0;
}
