#include "Precompiled.h"
#include "Engine.h"

Engine::App& Engine::MainApp()
{
	static App sApp;
	return sApp;
}
