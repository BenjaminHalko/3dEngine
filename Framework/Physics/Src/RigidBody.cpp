#include "Precompiled.h"
#include "RigidBody.h"

using namespace Engine;
using namespace Engine::Physics;

RigidBody::~RigidBody()
{
    Terminate();
}

void RigidBody::Initialize(Graphics::Transform& graphicsTransform,
                           const CollisionShape& shape,
                           float mass)
{
    mMass = mass;

    btTransform btTrans;
    btTrans.setIdentity();
    btTrans.setOrigin(ToBtVector3(graphicsTransform.position));
    btTrans.setRotation(ToBtQuaternion(graphicsTransform.rotation));

    mMotionState = new btDefaultMotionState(btTrans);

    btVector3 localInertia(0, 0, 0);
    if (mass > 0.0f)
    {
        shape.GetShape()->calculateLocalInertia(mass, localInertia);
    }

    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, mMotionState, shape.GetShape(), localInertia);
    mRigidBody = new btRigidBody(rbInfo);
}

void RigidBody::Terminate()
{
    delete mRigidBody;
    mRigidBody = nullptr;
    delete mMotionState;
    mMotionState = nullptr;
}

void RigidBody::SetPosition(const Math::Vector3& position)
{
    if (mRigidBody != nullptr)
    {
        btTransform& transform = mRigidBody->getWorldTransform();
        transform.setOrigin(ToBtVector3(position));
        mRigidBody->setWorldTransform(transform);

        if (mMotionState != nullptr)
        {
            mMotionState->setWorldTransform(transform);
        }

        mRigidBody->activate();
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

bool RigidBody::IsDynamic() const
{
    return mMass > 0.0f;
}

void RigidBody::SyncWithGraphics(Graphics::Transform& transform)
{
    if (mRigidBody != nullptr)
    {
        btTransform btTrans;
        if (mMotionState != nullptr)
        {
            mMotionState->getWorldTransform(btTrans);
        }
        else
        {
            btTrans = mRigidBody->getWorldTransform();
        }

        transform.position = ToVector3(btTrans.getOrigin());
        transform.rotation = ToQuaternion(btTrans.getRotation());
    }
}

btRigidBody* RigidBody::GetRigidBody()
{
    return mRigidBody;
}
