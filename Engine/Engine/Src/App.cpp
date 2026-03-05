#include "Precompiled.h"
#include "App.h"
#include "AppState.h"

using namespace Engine;
using namespace Engine::Core;
using namespace Engine::Graphics;
using namespace Engine::Input;
using namespace Engine::Audio;

void App::Run(const AppConfig& config)
{
    LOG("App Started");

    // Initialize Everything
    Window myWindow;
    myWindow.Initialize(nullptr, config.appName, config.winWidth, config.winHeight);
    auto handle = myWindow.GetWindowHandle();
    GraphicsSystem::StaticInitialize(handle, false);
    InputSystem::StaticInitialize(handle);
    DebugUI::StaticInitialize(handle, false, true);
    SimpleDraw::StaticInitialize(config.maxVertexCount);
    TextureManager::StaticInitialize(L"Assets/Textures");
    ModelManager::StaticInitialize(L"Assets/Models");
    Physics::PhysicsWorld::StaticInitialize();
    EventManager::StaticInitialize();
    AudioSystem::StaticInitialize();
    SoundEffectManager::StaticInitialize();
    SoundEffectManager::Get()->SetRootPath("Assets/Audio");

    // Last Step Before Running
    ASSERT(mCurrentState != nullptr, "App: Need an app state to run");
    mCurrentState->Initialize();

    // Process Updates
    InputSystem* input = InputSystem::Get();
    mRunning = true;
    while (mRunning)
    {
        myWindow.ProcessMessage();

        input->Update();

        if (!myWindow.IsActive() || input->IsKeyPressed(KeyCode::ESCAPE))
        {
            Quit();
            continue;
        }

        if (mNextState != nullptr)
        {
            mCurrentState->Terminate();
            mCurrentState = std::exchange(mNextState, nullptr);
            mCurrentState->Initialize();
        }

        float deltaTime = TimeUtil::GetDeltaTime();
#if defined(_DEBUG)
        if (deltaTime < 0.5f) // Primarily for handling Breakpoints
#endif
        {
            Physics::PhysicsWorld::Get()->Update(deltaTime);
            mCurrentState->Update(deltaTime);
        }

        GraphicsSystem* gs = GraphicsSystem::Get();
        gs->BeginRender();
        mCurrentState->Render();

        DebugUI::BeginRender();
        mCurrentState->DebugUI();
        DebugUI::EndRender();

        gs->EndRender();
    }

    // Terminate Everything
    LOG("App Quit");
    mCurrentState->Terminate();

    SoundEffectManager::StaticTerminate();
    AudioSystem::StaticTerminate();
    EventManager::StaticTerminate();
    Physics::PhysicsWorld::StaticTerminate();
    ModelManager::StaticTerminate();
    TextureManager::StaticTerminate();
    DebugUI::StaticTerminate();
    SimpleDraw::StaticTerminate();
    GraphicsSystem::StaticTerminate();
    InputSystem::StaticTerminate();

    myWindow.Terminate();
}

void App::Quit()
{
    mRunning = false;
}

void App::ChangeState(const std::string& stateName)
{
    auto iter = mAppStates.find(stateName);
    if (iter != mAppStates.end())
    {
        mNextState = iter->second.get();
    }
}
