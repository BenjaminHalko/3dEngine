#pragma once

#include <Engine/Inc/Engine.h>

class GameState : public Engine::AppState
{
public:
    void Initialize() override;

    void Terminate() override;

    void Update(float deltaTime) override;

    void Render() override;

    void DebugUI() override;

private:

    void UpdateCamera(float deltaTime);

    Engine::Graphics::Camera mCamera;
    Engine::Graphics::DirectionalLight mDirectionalLight;

    Engine::Graphics::RenderGroup mCharacter;
    Engine::Graphics::RenderGroup parasite;
    Engine::Graphics::RenderGroup zombie;
    Engine::Graphics::RenderObject mGround;

    Engine::Graphics::RenderObject mScreenQuad;

    Engine::Graphics::StandardEffect mStandardEffect;
    Engine::Graphics::ShadowEffect mShadowEffect;
};
