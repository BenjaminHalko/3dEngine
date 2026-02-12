#pragma once

namespace Engine::Physics
{
class CollisionShape final
{
  public:
    CollisionShape() = default;
    ~CollisionShape();

    void InitializeSphere(float radius);
    void InitializeBox(const Math::Vector3& halfExtents);
    void InitializeCapsule(float radius, float height);
    void InitializeHull(const Math::Vector3& halfExtents, const Math::Vector3& origin);
    void Terminate();

    btCollisionShape* GetShape() const { return mShape; }

  private:
    btCollisionShape* mShape = nullptr;
};
} // namespace Engine::Physics
