#include <Engine/Inc/Engine.h>

using namespace Engine;

class MainState : public AppState
{
public:
	void Initialize()
	{
		LOG("MAIN STATE INITIALIZED");
		mLifeTime = 2.0f;
	}
	void Terminate() override
	{
		LOG("MAIN STATE TERMINATED");
	}
	void Update(float deltaTime) override
	{
		mLifeTime -= std::max(deltaTime, 0.01f);
		if (mLifeTime <= 0.0f)
		{
			MainApp().ChangeState("GameState");
		}
	}
private:
	float mLifeTime = 0.0f;
};

class GameState : public AppState
{
public:
	void Initialize()
	{
		LOG("MAIN STATE INITIALIZED");
		mLifeTime = 2.0f;
	}
	void Terminate() override
	{
		LOG("MAIN STATE TERMINATED");
	}
	void Update(float deltaTime) override
	{
		mLifeTime -= std::max(deltaTime, 0.01f);
		if (mLifeTime <= 0.0f)
		{
			MainApp().ChangeState("MainState");
		}
	}
private:
	float mLifeTime = 0.0f;
};

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
	AppConfig config;
	config.appName = L"Hello Window";

	App& myApp = MainApp();
	myApp.AddState<MainState>("MainState");
	myApp.AddState<GameState>("GameState");
	myApp.Run(config);

	return 0;
}