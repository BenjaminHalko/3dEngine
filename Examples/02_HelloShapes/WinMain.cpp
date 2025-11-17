#include "ShapeState.h"
#include <Engine/Inc/Engine.h>

int main()
{
    Engine::AppConfig config;
    config.appName = L"Hello Shapes";

    config.winWidth = 1200;
    config.winHeight = 720;

    Engine::App& myApp = Engine::MainApp();

    myApp.AddState<TriForce>("TriForce");
    myApp.AddState<House>("House");
    myApp.AddState<Heart>("Heart");

    myApp.Run(config);

    return 0;
}
