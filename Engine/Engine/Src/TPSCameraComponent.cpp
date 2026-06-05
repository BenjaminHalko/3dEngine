#include "Precompiled.h"
#include "TPSCameraComponent.h"
#include "GameObject.h"
#include "TransformComponent.h"
#include "CameraComponent.h"
#include "SaveUtil.h"

using namespace Engine;

void TPSCameraComponent::Initialize()
{
    mTransformComponent = GetOwner().GetComponent<TransformComponent>();
    mCameraComponent = GetOwner().GetComponent<CameraComponent>();
    ASSERT(mTransformComponent != nullptr && mCameraComponent != nullptr,
           "TPSCameraComponent: Missing either transform or camera!");
}

void TPSCameraComponent::LateUpdate(float deltaTime)
{
    Graphics::Camera& camera = mCameraComponent->GetCamera();
    Math::Matrix4 matWorld = mTransformComponent->GetMatrix4();
    Math::Vector3 targetPosition = Math::TransformCoord(mOffset, matWorld);
    Math::Vector3 targetDirection = Math::GetLook(matWorld);
    targetPosition = Math::Lerp(camera.GetPosition(), targetPosition, deltaTime * mSmoothingValue);
    targetDirection =
        Math::Lerp(camera.GetDirection(), targetDirection, deltaTime * mSmoothingValue);
    camera.SetPosition(targetPosition);
    camera.SetDirection(targetDirection);
}

void TPSCameraComponent::DebugUI()
{
    ImGui::DragFloat3("Offset", &mOffset.x, 0.1f);
    ImGui::DragFloat("Smoothing", &mSmoothingValue, 0.1f, 0.1f, 100.0f);
}

void TPSCameraComponent::Deserialize(const rapidjson::Value& value)
{
    SaveUtil::ReadVector3("Offset", mOffset, value);
    SaveUtil::ReadFloat("Smoothing", mSmoothingValue, value);
}
