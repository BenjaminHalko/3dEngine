#include "precompiled.h"
#include "app.h"

using namespace Engine;
using namespace Engine::Core;

void App::Run(const AppConfig &config) {
    // Initialize Everything
    Window myWindow;
    myWindow.Initialize(
      GetModuleHandle(nullptr),
      config.appName,
      config.width,
      config.height
    );

    // Process Updates
    mRunning = true;
    while(mRunning) {
        myWindow.ProcessMessage();
        if (!myWindow.IsActive()) {
            Quit();
        }
    }

    // Terminate everything
    LOG("App Quit");
    myWindow.Terminate();
}

void App::Quit() {
    mRunning = false;
}
