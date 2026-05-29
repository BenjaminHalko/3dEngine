#include "Precompiled.h"
#include "PhysicsService.h"
#include "RigidBodyComponent.h"
#include "SaveUtil.h"

using namespace Engine;

void PhysicsService::Update(float deltaTime)
{
    if (mEnabled)
    {
        Physics::PhysicsWorld::Get()->Update(deltaTime);
    }
}

void PhysicsService::DebugUI()
{
    if (mEnabled)
    {
        Physics::PhysicsWorld::Get()->DebugUI();
    }
}

void PhysicsService::Deserialize(const rapidjson::Value& value)
{
    Math::Vector3 gravity{0.0f, -9.81f, 0.0f};
    if (SaveUtil::ReadVector3("Gravity", gravity, value))
    {
        Physics::PhysicsWorld::Get()->SetGravity(gravity);
    }
}

void PhysicsService::Register(RigidBodyComponent* rigidBodyComponent)
{
    Physics::PhysicsWorld::Get()->Register(&rigidBodyComponent->mRigidBody);
}

void PhysicsService::Unregister(RigidBodyComponent* rigidBodyComponent)
{
    Physics::PhysicsWorld::Get()->Unregister(&rigidBodyComponent->mRigidBody);
}

void PhysicsService::SetEnabled(bool enabled)
{
    mEnabled = enabled;
}
