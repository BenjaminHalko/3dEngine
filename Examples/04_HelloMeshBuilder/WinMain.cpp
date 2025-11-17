#include "ShapeState.h"
#include <Engine/Inc/Engine.h>

int main()
{
    Engine::AppConfig config;
    config.appName = L"Hello MeshBuilder";

    Engine::App& myApp = Engine::MainApp();

    myApp.AddState<ShapeState>("ShapeState");

    myApp.Run(config);

    return 0;
}
