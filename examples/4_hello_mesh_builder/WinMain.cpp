#include <Engine/Inc/Engine.h>
#include "ShapeState.h"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
    Engine::AppConfig config;
    config.appName = L"Hello MeshBuilder";

    Engine::App& myApp = Engine::MainApp();

    myApp.AddState<ShapeState>("ShapeState");

    myApp.Run(config);

    return 0;
}