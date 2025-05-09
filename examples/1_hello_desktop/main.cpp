#include <engine/inc/engine.h>

class MainState final : public Engine::AppState
{
public:
    void Initialize() override
    {
        LOG("MAIN STATE INITIALIZED");
        mLifeTime = 2.0f;
    }
    void Terminate() override
    {
        LOG("MAIN STATE TERMINATED");
    }
    void Update(const float deltaTime) override
    {
        mLifeTime -= std::max(deltaTime, 0.01f);
        if (mLifeTime <= 0.0f)
        {
            Engine::MainApp().ChangeState("GameState");
        }
    }
private:
    float mLifeTime = 0.0f;
};

class GameState final : public Engine::AppState
{
public:
    void Initialize() override
    {
        LOG("MAIN STATE INITIALIZED");
        mLifeTime = 2.0f;
    }
    void Terminate() override
    {
        LOG("MAIN STATE TERMINATED");
    }
    void Update(const float deltaTime) override
    {
        mLifeTime -= std::max(deltaTime, 0.01f);
        if (mLifeTime <= 0.0f)
        {
            Engine::MainApp().ChangeState("MainState");
        }
    }
private:
    float mLifeTime = 0.0f;
};

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int) {
    Engine::AppConfig config;
    config.appName = L"Hello Window";

    Engine::App& myApp = Engine::MainApp();
    myApp.AddState<MainState>("MainState");
    myApp.AddState<GameState>("GameState");
    myApp.Run(config);

    return 0;
}
