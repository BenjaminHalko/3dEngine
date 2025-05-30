#include "game_state.h"

using namespace Engine;

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
	AppConfig config;
	config.appName = L"Hello Debug";

	App& myApp = MainApp();

	myApp.AddState<game_state>("GameState");

	myApp.Run(config);

	return 0;
}