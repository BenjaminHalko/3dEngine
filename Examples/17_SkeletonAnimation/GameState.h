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
    Engine::Graphics::StandardEffect mStandardEffect;

    // Character models
    Engine::Graphics::RenderGroup mCharacter;
    Engine::Graphics::RenderGroup mParasite;
    Engine::Graphics::RenderGroup mZombie;

    // Animators
    Engine::Graphics::Animator mCharacterAnimator;
    Engine::Graphics::Animator mParasiteAnimator;
    Engine::Graphics::Animator mZombieAnimator;

    int mClipIndex = -1;
    float mAnimationSpeed = 1.0f;
    bool mDrawSkeleton = false;
};
