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
    void ResetCinematic();

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

    // Playback controls
    bool mPaused = false;
    bool mFreeCam = false;
    float mStartupDelay = 3.0f;

    // Per-shot timing (editable via sliders)
    float mShot1Start = 0.0f;
    float mShot2Start = 3.0f;
    float mShot3Start = 6.0f;
    float mShot4Start = 8.0f;
    float mShot5Start = 9.5f;
    float mShot6Start = 11.0f;
    float mShot7Start = 13.0f;
    float mShot8Start = 15.0f;
    float mShot9Start = 17.0f;
    float mCinematicEnd = 21.0f;

    // Audio
    Engine::Audio::SoundId mMusicId = 0;
};
