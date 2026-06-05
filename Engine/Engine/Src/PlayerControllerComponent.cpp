#include "Precompiled.h"
#include "PlayerControllerComponent.h"
#include "TransformComponent.h"
#include "RigidBodyComponent.h"
#include "GameObject.h"
#include "SaveUtil.h"

using namespace Engine;
using namespace Engine::Input;

void PlayerControllerComponent::Initialize()
{
    mRigidBodyComponent = GetOwner().GetComponent<RigidBodyComponent>();
    mTransformComponent = GetOwner().GetComponent<TransformComponent>();
    ASSERT(mRigidBodyComponent != nullptr || mTransformComponent != nullptr,
           "PlayerControllerComponent: No TransformComponent or RigidBodyComponent found!");
}

void PlayerControllerComponent::Terminate()
{
    mRigidBodyComponent = nullptr;
    mTransformComponent = nullptr;
}

void PlayerControllerComponent::Update(float deltaTime)
{
    Math::Vector2 moveInput = Math::Vector2::Zero;
    auto input = InputSystem::Get();
    const float moveSpeed = ((input->IsKeyDown(KeyCode::LSHIFT)) ? mShiftSpeed : mMoveSpeed);
    const float turnSpeed = mTurnSpeed;
    if (input->IsKeyDown(KeyCode::W))
    {
        moveInput.y = moveSpeed;
    }
    if (input->IsKeyDown(KeyCode::S))
    {
        moveInput.y = -moveSpeed;
    }
    if (input->IsKeyDown(KeyCode::D))
    {
        moveInput.x = moveSpeed;
    }
    if (input->IsKeyDown(KeyCode::A))
    {
        moveInput.x = -moveSpeed;
    }
    if (input->IsKeyDown(KeyCode::SPACE))
    {
        if (mRigidBodyComponent != nullptr)
        {
            Math::Vector3 vel = mRigidBodyComponent->GetVelocity();
            vel.y = mJumpSpeed;
            mRigidBodyComponent->SetVelocity(vel);
        }
    }

    float turnInput = 0.0f;
    if (input->IsMouseDown(MouseButton::RBUTTON))
    {
        turnInput = input->GetMouseMoveX() * turnSpeed;
    }

    Math::Matrix4 matrix = mTransformComponent->GetMatrix4();
    Math::Vector3 forward = Math::GetLook(matrix);
    Math::Vector3 right = Math::GetRight(matrix);
    forward = Math::Normalize({forward.x, 0.0f, forward.z});
    right = Math::Normalize({right.x, 0.0f, right.z});

    if (mRigidBodyComponent != nullptr)
    {
        Math::Vector3 vel = (forward * moveInput.y) + (right * moveInput.x);
        vel.y = mRigidBodyComponent->GetVelocity().y;
        mRigidBodyComponent->SetVelocity(vel);

        Math::Vector3 angularVel = mRigidBodyComponent->GetAngularVelocity();
        angularVel.y = turnInput;
        mRigidBodyComponent->SetAngularVelocity(angularVel);
    }
    else
    {
        mTransformComponent->position +=
            ((forward * moveInput.y) + (right * moveInput.x)) * deltaTime;

        mTransformComponent->rotation.y += turnInput * deltaTime;
        mTransformComponent->rotation = Math::Quaternion::Normalize(mTransformComponent->rotation);
    }
}

void PlayerControllerComponent::DebugUI()
{
    ImGui::DragFloat("MoveSpeed", &mMoveSpeed, 0.1f, 0.1f, 100.0f);
    ImGui::DragFloat("ShiftSpeed", &mShiftSpeed, 0.1f, 0.1f, 1000.0f);
    ImGui::DragFloat("TurnSpeed", &mTurnSpeed, 0.001f, 0.01f, 1.0f);
    ImGui::DragFloat("JumpSpeed", &mJumpSpeed, 0.001f, 0.1f, 1.0f);
}

void PlayerControllerComponent::Deserialize(const rapidjson::Value& value)
{
    SaveUtil::ReadFloat("MoveSpeed", mMoveSpeed, value);
    SaveUtil::ReadFloat("ShiftSpeed", mShiftSpeed, value);
    SaveUtil::ReadFloat("TurnSpeed", mTurnSpeed, value);
    SaveUtil::ReadFloat("JumpSpeed", mJumpSpeed, value);
}
