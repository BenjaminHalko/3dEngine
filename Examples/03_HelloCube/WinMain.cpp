#include "ShapeState.h"
#include <Engine/Inc/Engine.h>

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
    Engine::AppConfig config;
    config.appName = L"Hello Cube";

    Engine::App& myApp = Engine::MainApp();

    myApp.AddState<ShapeState>("ShapeState");
    myApp.AddState<CubeState>("Cube");
    myApp.AddState<PyramidState>("Pyramid");
    myApp.AddState<RectangleState>("Rectangle");

    myApp.Run(config);

    return 0;
}
