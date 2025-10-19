#include "Precompiled.h"
#include "Camera.h"
#include "GraphicsSystem.h"

using namespace Engine;
using namespace Engine::Graphics;

void Camera::SetMode(ProjectionMode mode) { mProjectionMode = mode; }

void Camera::SetPosition(const Math::Vector3 &position) { mPosition = position; }

void Camera::SetDirection(const Math::Vector3 &direction) {
    Math::Vector3 dir = Math::Normalize(direction); // Sanity Check to be safe
    if (Math::Abs(Math::Dot(dir, Math::Vector3::YAxis)) <
        0.995f) // Basically we aren't looking straight up
    {
        mDirection = dir;
    }
}

void Camera::SetLookAt(const Math::Vector3 &target) { SetDirection(target - mPosition); }

void Camera::SetFOV(float fov) {
    constexpr float kMinFov = 10.0f * Math::Constants::DegToRad;
    constexpr float kMaxFov = 170.0f * Math::Constants::DegToRad;

    mFov = Math::Clamp(fov, kMinFov, kMaxFov);
}

void Camera::SetAspectRatio(float ratio) { mAspectRatio = ratio; }

void Camera::SetSize(float width, float height) {
    mWidth = width;
    mHeight = height;
}

float Camera::GetSize() const { return mWidth; }

void Camera::SetNearPlane(float nearPlane) { mNearPlane = nearPlane; }

void Camera::SetFarPlane(float farPlane) { mFarPlane = farPlane; }

void Camera::Walk(float distance) { mPosition += mDirection * distance; }

void Camera::Strafe(float distance) {
    const Math::Vector3 right = Math::Normalize(Math::Cross(Math::Vector3::YAxis, mDirection));
    mPosition += right * distance;
}

void Camera::Rise(float distance) { mPosition += Math::Vector3::YAxis * distance; }

void Camera::Yaw(float radians) {
    const Math::Matrix4 matRotate = Math::Matrix4::RotationY(radians);
    const Math::Vector3 newLook = Math::TransformNormal(mDirection, matRotate);
    SetDirection(newLook);
}

void Camera::Pitch(float radians) {
    const Math::Vector3 right = Math::Normalize(Math::Cross(Math::Vector3::YAxis, mDirection));
    const Math::Matrix4 matRotate = Math::Matrix4::RotationAxis(right, radians);
    const Math::Vector3 newLook = Math::TransformNormal(mDirection, matRotate);
    SetDirection(newLook);
}

void Camera::Zoom(float amount) { SetFOV(mFov - amount); }

const Math::Vector3 &Camera::GetPosition() const { return mPosition; }

const Math::Vector3 &Camera::GetDirection() const { return mDirection; }

Math::Matrix4 Camera::GetViewMatrix() const {
    const Math::Vector3 l = mDirection;
    const Math::Vector3 r = Math::Normalize(Math::Cross(Math::Vector3::YAxis, mDirection));
    const Math::Vector3 u = Math::Normalize(Math::Cross(l, r));
    const float a = -Math::Dot(r, mPosition);
    const float b = -Math::Dot(u, mPosition);
    const float c = -Math::Dot(l, mPosition);

    return {r.x, u.x, l.x, 0.0f, r.y, u.y, l.y, 0.0f, r.z, u.z, l.z, 0.0f, a, b, c, 1.0f};
}

Math::Matrix4 Camera::GetProjectionMatrix() const {
    return (mProjectionMode == ProjectionMode::Perspective) ? GetPerspectiveMatrix()
                                                            : GetOrthographicMatrix();
}

Math::Matrix4 Camera::GetPerspectiveMatrix() const {
    const float a =
        (mAspectRatio == 0.0f) ? GraphicsSystem::Get()->GetBackBufferAspectRatio() : mAspectRatio;
    const float d = 1.0f / tanf(mFov * 0.5f);
    const float w = d / a;
    const float zf = mFarPlane;
    const float zn = mNearPlane;
    const float q = zf / (zf - zn);

    return {w,    0.0f, 0.0f, 0.0f, 0.0f, d,    0.0f,    0.0f,
            0.0f, 0.0f, q,    1.0f, 0.0f, 0.0f, -zn * q, 0.0f};
}

Math::Matrix4 Camera::GetOrthographicMatrix() const {
    const float w = (mWidth == 0.0f) ? GraphicsSystem::Get()->GetBackBufferWidth() : mWidth;
    const float h = (mHeight == 0.0f) ? GraphicsSystem::Get()->GetBackBufferHeight() : mHeight;
    const float f = mFarPlane;
    const float n = mNearPlane;

    return {2.0f / w, 0.0f, 0.0f,           0.0f, 0.0f, 2.0f / h, 0.0f,        0.0f,
            0.0f,     0.0f, 1.0f / (f - n), 0.0f, 0.0f, 0.0f,     n / (n - f), 1.0f};
}
