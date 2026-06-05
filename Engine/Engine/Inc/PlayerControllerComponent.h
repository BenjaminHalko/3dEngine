#pragma once

#include "Component.h"

namespace Engine
{
class TransformComponent;
class RigidBodyComponent;

class PlayerControllerComponent : public Component
{
  public:
    SET_TYPE_ID(ComponentId::PlayerController);

    void Initialize() override;
    void Terminate() override;
    void Update(float deltaTime) override;
    void DebugUI() override;
    void Deserialize(const rapidjson::Value& value) override;

  private:
    TransformComponent* mTransformComponent = nullptr;
    RigidBodyComponent* mRigidBodyComponent = nullptr;

    float mShiftSpeed = 10.0f;
    float mMoveSpeed = 2.0f;
    float mTurnSpeed = 0.1f;
    float mJumpSpeed = 20.0f;
};
} // namespace Engine
