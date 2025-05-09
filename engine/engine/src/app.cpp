#include "precompiled.h"
#include "app.h"

#include "app_state.h"

using namespace Engine;
using namespace Engine::Core;
using namespace Engine::Graphics;
using namespace Engine::Input;

void App::Run(const AppConfig& config) {
    LOG("App Started: %s", config.appName.c_str());

    Window myWindow;
    myWindow.Initialize(
        GetModuleHandle(nullptr),
        config.appName,
        config.winWidth,
        config.winHeight);
    const auto handle = myWindow.GetWindowHandle();
    GraphicsSystem::StaticInitialize(handle, false);
    InputSystem::StaticInitialize(handle);

    ASSERT(mCurrentState != nullptr, "App: need an app state to run");
    mCurrentState->Initialize();

    // Process updates
    InputSystem* input = InputSystem::Get();
    mRunning = true;
    while (mRunning) {
        myWindow.ProcessMessage();

        input->Update();

        if (!myWindow.IsActive() || input->IsKeyPressed(KeyCode::ESCAPE)) {
            Quit();
            continue;
        }

        if (mNextState != nullptr) {
            mCurrentState->Terminate();
            mCurrentState = std::exchange(mNextState, nullptr);
            mCurrentState->Initialize();
        }

        const float deltaTime = TimeUtil::GetDeltaTime();
#if defined(_DEBUG)
        if (deltaTime < 0.5f)
#endif
        {
            mCurrentState->Update(deltaTime);
        }

        GraphicsSystem* gs = GraphicsSystem::Get();
        gs->BeginRender();
        mCurrentState->Render();
        gs->EndRender();
    }

    LOG("App Quit");
    mCurrentState->Terminate();

    GraphicsSystem::StaticTerminate();
    InputSystem::StaticTerminate();
    
    myWindow.Terminate();
}

void App::Quit() {
    mRunning = false;
}

void App::ChangeState(const std::string& stateName) {
    if (const auto iter = mAppStates.find(stateName); iter != mAppStates.end()) {
        mNextState = iter->second.get();
    }
}
