#pragma once

#include <engine/inc/engine.h>

namespace Engine
{
    class GameState : public AppState
    {
    public:
        void Initialize() override;
        void Terminate() override;
        void Update(float deltaTime) override;
        void Render() override;
        void DebugUI() override;

    private:
        void UpdateCamera(float deltaTime);

        Graphics::Camera mCamera;
        Graphics::DirectionalLight mDirectionalLight;
        Graphics::RenderObject mRenderObject;
        Graphics::StandardEffect mStandardEffect;
    };
}
