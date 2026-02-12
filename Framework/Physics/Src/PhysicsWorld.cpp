#include "Precompiled.h"
#include "PhysicsWorld.h"
#include "PhysicsObject.h"

using namespace Engine;
using namespace Engine::Physics;

namespace
{
std::unique_ptr<PhysicsWorld> sPhysicsWorld;
}

void PhysicsWorld::StaticInitialize(const Settings& settings)
{
    ASSERT(sPhysicsWorld == nullptr, "PhysicsWorld already initialized.");
    sPhysicsWorld = std::make_unique<PhysicsWorld>();
    sPhysicsWorld->Initialize(settings);
}

void PhysicsWorld::StaticTerminate()
{
    if (sPhysicsWorld != nullptr)
    {
        sPhysicsWorld->Terminate();
        sPhysicsWorld.reset();
    }
}

PhysicsWorld* PhysicsWorld::Get()
{
    ASSERT(sPhysicsWorld != nullptr, "PhysicsWorld not initialized.");
    return sPhysicsWorld.get();
}

PhysicsWorld::~PhysicsWorld()
{
    Terminate();
}

void PhysicsWorld::Initialize(const Settings& settings)
{
    mSettings = settings;

    mCollisionConfiguration = new btDefaultCollisionConfiguration();
    mDispatcher = new btCollisionDispatcher(mCollisionConfiguration);
    mBroadphase = new btDbvtBroadphase();
    mSolver = new btSequentialImpulseConstraintSolver();
    mDynamicsWorld = new btDiscreteDynamicsWorld(mDispatcher, mBroadphase, mSolver, mCollisionConfiguration);

    mDynamicsWorld->setGravity(ToBtVector3(settings.gravity));
}

void PhysicsWorld::Terminate()
{
    for (auto* obj : mPhysicsObjects)
    {
        btRigidBody* rb = obj->GetRigidBody();
        if (rb != nullptr)
        {
            mDynamicsWorld->removeRigidBody(rb);
        }
    }
    mPhysicsObjects.clear();

    delete mDynamicsWorld;
    mDynamicsWorld = nullptr;
    delete mSolver;
    mSolver = nullptr;
    delete mBroadphase;
    mBroadphase = nullptr;
    delete mDispatcher;
    mDispatcher = nullptr;
    delete mCollisionConfiguration;
    mCollisionConfiguration = nullptr;
}

void PhysicsWorld::Update(float deltaTime)
{
    if (mDynamicsWorld == nullptr)
    {
        return;
    }

    mDynamicsWorld->stepSimulation(deltaTime, mSettings.simulationSteps, mSettings.fixedTimeStep);

    for (auto* obj : mPhysicsObjects)
    {
        // Objects sync themselves externally
    }
}

void PhysicsWorld::Register(PhysicsObject* physicsObject)
{
    btRigidBody* rb = physicsObject->GetRigidBody();
    if (rb != nullptr && mDynamicsWorld != nullptr)
    {
        mDynamicsWorld->addRigidBody(rb);
        mPhysicsObjects.push_back(physicsObject);
    }
}

void PhysicsWorld::Unregister(PhysicsObject* physicsObject)
{
    btRigidBody* rb = physicsObject->GetRigidBody();
    if (rb != nullptr && mDynamicsWorld != nullptr)
    {
        mDynamicsWorld->removeRigidBody(rb);
    }

    auto it = std::find(mPhysicsObjects.begin(), mPhysicsObjects.end(), physicsObject);
    if (it != mPhysicsObjects.end())
    {
        mPhysicsObjects.erase(it);
    }
}

void PhysicsWorld::DebugUI()
{
    if (ImGui::CollapsingHeader("Physics"))
    {
        ImGui::DragFloat3("Gravity", &mSettings.gravity.x, 0.1f);
        if (mDynamicsWorld != nullptr)
        {
            mDynamicsWorld->setGravity(ToBtVector3(mSettings.gravity));
        }
    }
}
