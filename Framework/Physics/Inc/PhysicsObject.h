#pragma once

class btRigidBody;

namespace Engine::Physics
{
class PhysicsObject
{
  public:
    PhysicsObject() = default;
    virtual ~PhysicsObject() = default;

    virtual void SyncWithGraphics(Graphics::Transform& transform) = 0;
    virtual btRigidBody* GetRigidBody() = 0;
};
} // namespace Engine::Physics
