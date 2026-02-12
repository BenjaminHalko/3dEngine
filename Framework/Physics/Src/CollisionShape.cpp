#include "Precompiled.h"
#include "CollisionShape.h"

using namespace Engine;
using namespace Engine::Physics;

CollisionShape::~CollisionShape()
{
    Terminate();
}

void CollisionShape::InitializeSphere(float radius)
{
    ASSERT(mShape == nullptr, "CollisionShape already initialized.");
    mShape = new btSphereShape(radius);
}

void CollisionShape::InitializeBox(const Math::Vector3& halfExtents)
{
    ASSERT(mShape == nullptr, "CollisionShape already initialized.");
    mShape = new btBoxShape(ToBtVector3(halfExtents));
}

void CollisionShape::InitializeCapsule(float radius, float height)
{
    ASSERT(mShape == nullptr, "CollisionShape already initialized.");
    mShape = new btCapsuleShape(radius, height);
}

void CollisionShape::InitializeHull(const Math::Vector3& halfExtents, const Math::Vector3& origin)
{
    ASSERT(mShape == nullptr, "CollisionShape already initialized.");
    btConvexHullShape* hullShape = new btConvexHullShape();

    float hx = halfExtents.x;
    float hy = halfExtents.y;
    float hz = halfExtents.z;
    float ox = origin.x;
    float oy = origin.y;
    float oz = origin.z;

    // Add 8 corners of the box
    hullShape->addPoint(btVector3(ox - hx, oy - hy, oz - hz));
    hullShape->addPoint(btVector3(ox + hx, oy - hy, oz - hz));
    hullShape->addPoint(btVector3(ox - hx, oy + hy, oz - hz));
    hullShape->addPoint(btVector3(ox + hx, oy + hy, oz - hz));
    hullShape->addPoint(btVector3(ox - hx, oy - hy, oz + hz));
    hullShape->addPoint(btVector3(ox + hx, oy - hy, oz + hz));
    hullShape->addPoint(btVector3(ox - hx, oy + hy, oz + hz));
    hullShape->addPoint(btVector3(ox + hx, oy + hy, oz + hz));

    mShape = hullShape;
}

void CollisionShape::Terminate()
{
    delete mShape;
    mShape = nullptr;
}
