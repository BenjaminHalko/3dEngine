#pragma once

#include <Core/Inc/Core.h>
#include <Math/Inc/DWMath.h>
#include <Graphics/Inc/Graphics.h>

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

namespace Engine::Physics
{
inline btVector3 ToBtVector3(const Math::Vector3& v)
{
    return btVector3(v.x, v.y, v.z);
}

inline Math::Vector3 ToVector3(const btVector3& v)
{
    return Math::Vector3(v.x(), v.y(), v.z());
}

inline btQuaternion ToBtQuaternion(const Math::Quaternion& q)
{
    return btQuaternion(q.x, q.y, q.z, q.w);
}

inline Math::Quaternion ToQuaternion(const btQuaternion& q)
{
    return Math::Quaternion(q.x(), q.y(), q.z(), q.w());
}

inline Math::Matrix4 ToMatrix4(const btTransform& t)
{
    float m[16];
    t.getOpenGLMatrix(m);
    return Math::Matrix4(
        m[0], m[1], m[2], m[3],
        m[4], m[5], m[6], m[7],
        m[8], m[9], m[10], m[11],
        m[12], m[13], m[14], m[15]);
}
} // namespace Engine::Physics
