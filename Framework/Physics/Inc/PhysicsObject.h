#pragma once

class btRigidBody;
class btSoftBody;

namespace Engine::Physics
{
class PhysicsObject
{
  public:
    PhysicsObject() = default;
    virtual ~PhysicsObject() = default;

  protected:
    friend class PhysicsWorld;

    virtual void SyncWithGraphics() = 0;
    virtual btRigidBody* GetRigidBody()
    {
        return nullptr;
    }
    virtual btSoftBody* GetSoftBody()
    {
        return nullptr;
    }
};
} // namespace Engine::Physics
