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
    void OnShotEnter(int shot, const Engine::Graphics::Transform& smoothT);

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
    Engine::Math::Vector3 mDiveStartPos;

    // Cinematic sequencer
    float mCinematicTime = 0.0f;
    bool mCinematicDone = false;
    int mCurrentShot = 1;
};
