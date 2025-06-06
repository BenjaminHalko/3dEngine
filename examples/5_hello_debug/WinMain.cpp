#include "GameState.h"

using namespace Engine;

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int) {
    AppConfig config;
    config.appName = L"Hello Debug";

    App &myApp = MainApp();

    myApp.AddState<GameState>("GameState");

    myApp.Run(config);

    return 0;
}