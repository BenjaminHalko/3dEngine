#include <engine/inc/engine.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    Engine::AppConfig config;
    config.appName = L"Hello World";

    Engine::App app;
    app.Run(config);
    return 0;
}
