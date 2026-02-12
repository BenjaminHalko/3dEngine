#pragma once

namespace Engine::Physics
{
class PhysicsObject;

struct Settings
{
    Math::Vector3 gravity = {0.0f, -9.81f, 0.0f};
    uint32_t simulationSteps = 10;
    float fixedTimeStep = 1.0f / 60.0f;
};

class PhysicsWorld final
{
  public:
    static void StaticInitialize(const Settings& settings = {});
    static void StaticTerminate();
    static PhysicsWorld* Get();

    PhysicsWorld() = default;
    ~PhysicsWorld();

    void Initialize(const Settings& settings);
    void Terminate();

    void Update(float deltaTime);

    void Register(PhysicsObject* physicsObject);
    void Unregister(PhysicsObject* physicsObject);

    void DebugUI();

  private:
    Settings mSettings;

    btBroadphaseInterface* mBroadphase = nullptr;
    btDefaultCollisionConfiguration* mCollisionConfiguration = nullptr;
    btCollisionDispatcher* mDispatcher = nullptr;
    btSequentialImpulseConstraintSolver* mSolver = nullptr;
    btDiscreteDynamicsWorld* mDynamicsWorld = nullptr;

    using PhysicsObjects = std::vector<PhysicsObject*>;
    PhysicsObjects mPhysicsObjects;
};
} // namespace Engine::Physics
