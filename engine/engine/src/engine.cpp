#include "precompiled.h"
#include "engine.h"

Engine::App& Engine::MainApp() {
    static App sApp;
    return sApp;
}
