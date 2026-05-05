#include "Precompiled.h"
#include "CameraComponent.h"
#include "SaveUtil.h"
#include "CameraService.h"
#include "GameWorld.h"
#include "GameObject.h"

using namespace Engine;

void CameraComponent::Initialize()
{
    CameraService* cameraService = GetOwner().GetWorld().GetService<CameraService>();
    if (cameraService != nullptr)
    {
        cameraService->Register(this);
    }
}

void CameraComponent::Terminate()
{
    CameraService* cameraService = GetOwner().GetWorld().GetService<CameraService>();
    if (cameraService != nullptr)
    {
        cameraService->Unregister(this);
    }
}

void CameraComponent::DebugUI()
{
    Math::Vector3 pos = mCamera.GetPosition();
    Math::Vector3 dir = mCamera.GetDirection();
    if (ImGui::DragFloat3("Position##Camera", &pos.x, 0.1f))
    {
        mCamera.SetPosition(pos);
    }
    if (ImGui::DragFloat3("Direction##Camera", &dir.x, 0.1f))
    {
        mCamera.SetDirection(dir);
    }
}

void CameraComponent::Deserialize(const rapidjson::Value& value)
{
    Math::Vector3 readValue = Math::Vector3::Zero;
    if (SaveUtil::ReadVector3("Position", readValue, value))
    {
        mCamera.SetPosition(readValue);
    }
    if (SaveUtil::ReadVector3("LookAt", readValue, value))
    {
        mCamera.SetLookAt(readValue);
    }
    if (SaveUtil::ReadVector3("Direction", readValue, value))
    {
        mCamera.SetDirection(readValue);
    }
}

Graphics::Camera& CameraComponent::GetCamera()
{
    return mCamera;
}

const Graphics::Camera& CameraComponent::GetCamera() const
{
    return mCamera;
}
