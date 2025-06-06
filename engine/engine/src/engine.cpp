#include "Precompiled.h"

Engine::App &Engine::MainApp() {
    static App sApp;
    return sApp;
}
