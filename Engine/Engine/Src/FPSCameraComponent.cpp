#include "Precompiled.h"
#include "FPSCameraComponent.h"
#include "CameraComponent.h"
#include "GameObject.h"
#include "SaveUtil.h"

using namespace Engine;
using namespace Engine::Input;

void FPSCameraComponent::Initialize()
{
    mCameraComponent = GetOwner().GetComponent<CameraComponent>();
    ASSERT(mCameraComponent != nullptr, "FPSCameraComponent: Camera was not found!");
}

void FPSCameraComponent::Terminate()
{
    mCameraComponent = nullptr;
}

void FPSCameraComponent::Update(float deltaTime)
{
    Graphics::Camera& camera = mCameraComponent->GetCamera();
    auto input = InputSystem::Get();
    const float moveSpeed = ((input->IsKeyDown(KeyCode::LSHIFT)) ? mShiftSpeed : mMoveSpeed) * deltaTime;
    const float turnSpeed = mTurnSpeed * deltaTime;
    if (input->IsKeyDown(KeyCode::W))
    {
        camera.Walk(moveSpeed);
    }
    if (input->IsKeyDown(KeyCode::S))
    {
        camera.Walk(-moveSpeed);
    }
    if (input->IsKeyDown(KeyCode::D))
    {
        camera.Strafe(moveSpeed);
    }
    if (input->IsKeyDown(KeyCode::A))
    {
        camera.Strafe(-moveSpeed);
    }
    if (input->IsKeyDown(KeyCode::E))
    {
        camera.Rise(moveSpeed);
    }
    if (input->IsKeyDown(KeyCode::Q))
    {
        camera.Rise(-moveSpeed);
    }
    if (input->IsMouseDown(MouseButton::RBUTTON))
    {
        camera.Yaw(input->GetMouseMoveX() * turnSpeed);
        camera.Pitch(input->GetMouseMoveY() * turnSpeed);
    }
}

void FPSCameraComponent::DebugUI()
{
    ImGui::DragFloat("Move Speed", &mMoveSpeed, 0.1f, 0.1f, 100.0f);
    ImGui::DragFloat("Shift Speed", &mShiftSpeed, 0.1f, 0.1f, 1000.0f);
    ImGui::DragFloat("Turn Speed", &mTurnSpeed, 0.001f, 0.01f, 1.0f);
}

void FPSCameraComponent::Deserialize(const rapidjson::Value& value)
{
    SaveUtil::ReadFloat("MoveSpeed", mMoveSpeed, value);
    SaveUtil::ReadFloat("ShiftSpeed", mShiftSpeed, value);
    SaveUtil::ReadFloat("TurnSpeed", mTurnSpeed, value);
}
