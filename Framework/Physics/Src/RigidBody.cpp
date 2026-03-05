#include "Precompiled.h"
#include "RigidBody.h"

#include "CollisionShape.h"
#include "PhysicsWorld.h"

using namespace Engine;
using namespace Engine::Physics;

RigidBody::~RigidBody()
{
    ASSERT(mRigidBody == nullptr, "RigidBody: Terminate must be called!");
}

void RigidBody::Initialize(Graphics::Transform& graphicsTransform,
                           const CollisionShape& shape,
                           float mass,
                           bool addToWorld)
{
    mMass = mass;
    mGraphicsTransform = &graphicsTransform;

    mMotionState = new btDefaultMotionState(ConvertToBtTransform(graphicsTransform));

    btVector3 localInertia(0, 0, 0);
    if (mass > 0.0f)
    {
        shape.GetShape()->calculateLocalInertia(mass, localInertia);
    }

    btRigidBody::btRigidBodyConstructionInfo rbInfo(
        mass, mMotionState, shape.GetShape(), localInertia);
    mRigidBody = new btRigidBody(rbInfo);

    if (addToWorld)
    {
        PhysicsWorld::Get()->Register(this);
    }
}

void RigidBody::Terminate()
{
    PhysicsWorld::Get()->Unregister(this);

    delete mRigidBody;
    mRigidBody = nullptr;
    delete mMotionState;
    mMotionState = nullptr;
}

void RigidBody::SetPosition(const Math::Vector3& position)
{
    if (mRigidBody != nullptr)
    {
        mRigidBody->activate();
        mGraphicsTransform->position = position;
        mRigidBody->setWorldTransform(ConvertToBtTransform(*mGraphicsTransform));
        if (mMotionState != nullptr)
            mMotionState->setWorldTransform(ConvertToBtTransform(*mGraphicsTransform));
    }
}

void RigidBody::SetVelocity(const Math::Vector3& velocity)
{
    if (mRigidBody != nullptr)
    {
        mRigidBody->setLinearVelocity(ToBtVector3(velocity));
        mRigidBody->activate();
    }
}

void RigidBody::Activate()
{
    PhysicsWorld::Get()->Register(this);
    mRigidBody->activate();
}

void RigidBody::Deactivate()
{
    PhysicsWorld::Get()->Unregister(this);
}

void RigidBody::SetCollisionFlags(int flags)
{
    mRigidBody->setCollisionFlags(flags);
}

const Math::Vector3 RigidBody::GetVelocity() const
{
    return ToVector3(mRigidBody->getLinearVelocity());
}

bool RigidBody::IsDynamic() const
{
    return mMass > 0.0f;
}

void RigidBody::SyncWithGraphics()
{
    const btTransform& worldTransform = mRigidBody->getWorldTransform();
    mGraphicsTransform->position = ToVector3(worldTransform.getOrigin());
    mGraphicsTransform->rotation = ToQuaternion(worldTransform.getRotation());
}

btRigidBody* RigidBody::GetRigidBody()
{
    return mRigidBody;
}
