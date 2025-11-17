#include "GameState.h"

using namespace Engine;

int main()
{
    AppConfig config;
    config.appName = L"Solar System";

    App& myApp = MainApp();

    myApp.AddState<GameState>("GameState");

    myApp.Run(config);

    return 0;
}
