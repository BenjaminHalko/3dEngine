#pragma once

#include "RigidBody.h"
#include "CollisionShape.h"

namespace Engine::Physics
{
struct ParticleInfo
{
    float lifetime = 0.0f;
    Math::Vector3 position = Math::Vector3::Zero;
    Math::Vector3 velocity = Math::Vector3::Zero;
    Graphics::Color startColour = Graphics::Colors::White;
    Graphics::Color endColour = Graphics::Colors::White;
    Math::Vector3 startScale = Math::Vector3::One;
    Math::Vector3 endScale = Math::Vector3::One;
};

class Particle
{
  public:
    void Initialize();
    void Terminate();
    void Activate(const ParticleInfo& info);
    void Update(float deltaTime);

    bool IsActive() const;

    const Graphics::Transform& GetTransform() const;
    Graphics::Color GetColour() const;
    const Math::Vector3& GetPosition() const;

  private:
    Graphics::Transform mTransform;
    RigidBody mRigidBody;
    CollisionShape mCollisionShape;

    ParticleInfo mInfo;
    float mLifetime = 0.0f;
};
} // namespace Engine::Physics
