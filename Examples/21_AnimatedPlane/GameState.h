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

    // Stanley
    Engine::Graphics::RenderGroup mStanley;
    Engine::Graphics::Animator mStanleyAnimator;

    // Ground
    Engine::Graphics::RenderObject mGround;

    // Plane
    Engine::Graphics::RenderGroup mPlane;
    Engine::Graphics::Animation mPlaneFlightAnimation;
    float mPlaneAnimTime = 0.0f;

    // Explosion particles
    Engine::Graphics::ParticleSystemEffect mParticleSystemEffect;
    Engine::Physics::ParticleSystem mExplosion;

    // Shaking state
    bool mPlaneShaking = false;
    float mShakeTime = 0.0f;

    // Explosion trigger
    float mSceneTimer = 0.0f;
    bool mExplosionTriggered = false;
};
