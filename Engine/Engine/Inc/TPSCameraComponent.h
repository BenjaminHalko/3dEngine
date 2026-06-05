#pragma once

#include "Component.h"

namespace Engine
{
class CameraComponent;
class TransformComponent;

class TPSCameraComponent : public Component
{
  public:
    SET_TYPE_ID(ComponentId::TPCamera);

    void Initialize() override;
    void LateUpdate(float deltaTime) override;
    void DebugUI() override;
    void Deserialize(const rapidjson::Value& value) override;

  private:
    const TransformComponent* mTransformComponent = nullptr;
    CameraComponent* mCameraComponent = nullptr;

    Math::Vector3 mOffset = Math::Vector3::Zero;
    float mSmoothingValue = 0.0f;
};
} // namespace Engine
