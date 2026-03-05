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
    void OnSpacePressedEvent(const Engine::Core::Event& e);

    Engine::Graphics::Camera mCamera;
    Engine::Graphics::DirectionalLight mDirectionalLight;
    Engine::Graphics::StandardEffect mStandardEffect;

    // Events
    Engine::Core::EventListenerId mSpacePressedListenerId = 0;
    Engine::Core::EventListenerId mEnterPressedListenerId = 0;

    // Particles
    Engine::Graphics::ParticleSystemEffect mParticleSystemEffect;
    Engine::Physics::ParticleSystem mParticleSystem;
    Engine::Physics::ParticleSystem mFireworkParticles;

    // Animation
    Engine::Graphics::Animation mFireworkAnimation;
    float mFireworkAnimationTime = 0.0f;

    // Football
    Engine::Graphics::RenderObject mFootball;
    Engine::Physics::CollisionShape mBallShape;
    Engine::Physics::RigidBody mBallRigidBody;

    // Ground
    Engine::Graphics::RenderObject mGroundObject;
    Engine::Physics::CollisionShape mGroundShape;
    Engine::Physics::RigidBody mGroundRigidBody;

    // Boxes
    struct BoxData
    {
        Engine::Graphics::RenderObject box;
        Engine::Physics::CollisionShape shape;
        Engine::Physics::RigidBody rigidBody;
    };
    std::vector<BoxData> mBoxes;

    // Cloth
    Engine::Graphics::RenderObject mClothRenderObject;
    Engine::Graphics::Mesh mClothMesh;
    Engine::Physics::SoftBody mClothSoftBody;

    // Sound
    Engine::Audio::SoundId mLaunchSoundId = 0;
    Engine::Audio::SoundId mExplosionSoundId = 0;
};
