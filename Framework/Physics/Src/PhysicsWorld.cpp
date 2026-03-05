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
    mBroadphase = new btDbvtBroadphase();
    mSolver = new btSequentialImpulseConstraintSolver();
#ifdef USE_SOFT_BODY
    mCollisionConfiguration = new btSoftBodyRigidBodyCollisionConfiguration();
#else
    mCollisionConfiguration = new btDefaultCollisionConfiguration();
#endif

    mDispatcher = new btCollisionDispatcher(mCollisionConfiguration);

#ifdef USE_SOFT_BODY
    mDynamicsWorld = new btSoftRigidDynamicsWorld(mDispatcher, mBroadphase, mSolver, mCollisionConfiguration);
#else
    mDynamicsWorld = new btDiscreteDynamicsWorld(mDispatcher, mBroadphase, mSolver, mCollisionConfiguration);
#endif

    mDynamicsWorld->setGravity(ToBtVector3(settings.gravity));
    mDynamicsWorld->setDebugDrawer(&mPhysicsDebugDraw);
}

void PhysicsWorld::Terminate()
{
    for (auto* obj : mPhysicsObjects)
    {
#ifdef USE_SOFT_BODY
        if (obj->GetSoftBody() != nullptr)
            mDynamicsWorld->removeSoftBody(obj->GetSoftBody());
#endif
        if (obj->GetRigidBody() != nullptr)
            mDynamicsWorld->removeRigidBody(obj->GetRigidBody());
    }
    mPhysicsObjects.clear();
    delete mDynamicsWorld; mDynamicsWorld = nullptr;
    delete mSolver; mSolver = nullptr;
    delete mDispatcher; mDispatcher = nullptr;
    delete mCollisionConfiguration; mCollisionConfiguration = nullptr;
    delete mBroadphase; mBroadphase = nullptr;
}

void PhysicsWorld::Update(float deltaTime)
{
    if (mDynamicsWorld == nullptr) return;
    mDynamicsWorld->stepSimulation(deltaTime, mSettings.simulationSteps, mSettings.fixedTimeStep);
    for (PhysicsObject* obj : mPhysicsObjects)
        obj->SyncWithGraphics();
}

void PhysicsWorld::Register(PhysicsObject* physicsObject)
{
    auto iter = std::find(mPhysicsObjects.begin(), mPhysicsObjects.end(), physicsObject);
    if (iter == mPhysicsObjects.end())
    {
        mPhysicsObjects.push_back(physicsObject);
#ifdef USE_SOFT_BODY
        if (physicsObject->GetSoftBody() != nullptr)
            mDynamicsWorld->addSoftBody(physicsObject->GetSoftBody());
#endif
        if (physicsObject->GetRigidBody() != nullptr)
            mDynamicsWorld->addRigidBody(physicsObject->GetRigidBody());
    }
}

void PhysicsWorld::Unregister(PhysicsObject* physicsObject)
{
    auto iter = std::find(mPhysicsObjects.begin(), mPhysicsObjects.end(), physicsObject);
    if (iter != mPhysicsObjects.end())
    {
        #ifdef USE_SOFT_BODY
        if (physicsObject->GetSoftBody() != nullptr)
            mDynamicsWorld->removeSoftBody(physicsObject->GetSoftBody());
    #endif
        if (physicsObject->GetRigidBody() != nullptr)
            mDynamicsWorld->removeRigidBody(physicsObject->GetRigidBody());
        mPhysicsObjects.erase(iter);
    }
}

void PhysicsWorld::DebugUI()
{
    if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::DragFloat3("Gravity", &mSettings.gravity.x, 0.1f))
            mDynamicsWorld->setGravity(ToBtVector3(mSettings.gravity));
        ImGui::Checkbox("DebugDraw", &mDebugDraw);
        if (mDebugDraw)
        {
            ImGui::Indent();
            int debugMode = mPhysicsDebugDraw.getDebugMode();
            bool drawWireframe = (debugMode & btIDebugDraw::DBG_DrawWireframe);
            if (ImGui::Checkbox("DrawWireframe", &drawWireframe))
                debugMode = drawWireframe ? (debugMode | btIDebugDraw::DBG_DrawWireframe) : (debugMode & ~btIDebugDraw::DBG_DrawWireframe);
            bool drawAabb = (debugMode & btIDebugDraw::DBG_DrawAabb);
            if (ImGui::Checkbox("DrawAabb", &drawAabb))
                debugMode = drawAabb ? (debugMode | btIDebugDraw::DBG_DrawAabb) : (debugMode & ~btIDebugDraw::DBG_DrawAabb);
            bool drawContactPoints = (debugMode & btIDebugDraw::DBG_DrawContactPoints);
            if (ImGui::Checkbox("DrawContactPoints", &drawContactPoints))
                debugMode = drawContactPoints ? (debugMode | btIDebugDraw::DBG_DrawContactPoints) : (debugMode & ~btIDebugDraw::DBG_DrawContactPoints);
            mPhysicsDebugDraw.setDebugMode(debugMode);
            mDynamicsWorld->debugDrawWorld();
            ImGui::Unindent();
        }
    }
}

void PhysicsWorld::SetGravity(const Math::Vector3& gravity)
{
    mSettings.gravity = gravity;
    mDynamicsWorld->setGravity(ToBtVector3(gravity));
}
