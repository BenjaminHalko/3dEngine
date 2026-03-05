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
                    float mass = 0.0f,
                    bool addToWorld = true);
    void Terminate();

    void SetPosition(const Math::Vector3& position);
    void SetVelocity(const Math::Vector3& velocity);
    void Activate();
    void Deactivate();
    void SetCollisionFlags(int flags);
    const Math::Vector3 GetVelocity() const;

    bool IsDynamic() const;

  private:
    void SyncWithGraphics() override;
    btRigidBody* GetRigidBody() override;

    btRigidBody* mRigidBody = nullptr;
    btDefaultMotionState* mMotionState = nullptr;
    float mMass = 0.0f;
    Graphics::Transform* mGraphicsTransform = nullptr;
};
} // namespace Engine::Physics
