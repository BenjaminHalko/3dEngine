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

    UIFont::StaticInitialize(UIFont::FontType::Verdana);
    UISpriteRenderer::StaticInitialize();

    ASSERT(mCurrentState != nullptr, "App: Need an app state to run");
    mCurrentState->Initialize();

    InputSystem* input = InputSystem::Get();
    mRunning = true;
    while (mRunning)
    {
        myWindow.ProcessMessage();

        input->Update();

        if (!myWindow.IsActive() || (mQuitOnEscape && input->IsKeyPressed(KeyCode::ESCAPE)))
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

        AudioSystem::Get()->Update();

        float deltaTime = TimeUtil::GetDeltaTime();
#if defined(_DEBUG)
        if (deltaTime < 0.5f)
#endif
        {
            mCurrentState->Update(deltaTime);
        }

        GraphicsSystem* gs = GraphicsSystem::Get();
        gs->BeginRender();
        DebugUI::BeginRender();
        mCurrentState->Render();
        mCurrentState->DebugUI();
        DebugUI::EndRender();

        gs->EndRender();
    }

    LOG("App Quit");
    mCurrentState->Terminate();

    UISpriteRenderer::StaticTerminate();
    UIFont::StaticTerminate();
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
