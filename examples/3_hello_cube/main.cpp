#include <engine/inc/engine.h>
#include "shape_state.h"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
    Engine::AppConfig config;
    config.appName = L"Hello Cube";

    Engine::App& myApp = Engine::MainApp();
    myApp.AddState<ShapeState>("ShapeState");
    myApp.Run(config);

    return 0;
} 