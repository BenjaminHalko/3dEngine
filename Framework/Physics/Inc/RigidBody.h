#pragma once

#include "PhysicsObject.h"
#include "CollisionShape.h"

namespace Engine::Physics
{
class RigidBody final : public PhysicsObject
{
  public:
    RigidBody() = default;
    ~RigidBody() override;

    void Initialize(Graphics::Transform& graphicsTransform,
                    const CollisionShape& shape,
                    float mass = 0.0f);
    void Terminate();

    void SetPosition(const Math::Vector3& position);
    void SetVelocity(const Math::Vector3& velocity);

    bool IsDynamic() const;

    void SyncWithGraphics(Graphics::Transform& transform) override;
    btRigidBody* GetRigidBody() override;

  private:
    btRigidBody* mRigidBody = nullptr;
    btDefaultMotionState* mMotionState = nullptr;
    float mMass = 0.0f;
};
} // namespace Engine::Physics
