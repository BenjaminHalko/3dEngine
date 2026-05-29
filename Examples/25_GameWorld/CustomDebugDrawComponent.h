#pragma once

#include "CustomTypeIds.h"
#include <Engine/Inc/Engine.h>

class CustomDebugDrawComponent : public Engine::Component
{
  public:
    SET_TYPE_ID(CustomComponentId::CustomDebugDraw);

    void Initialize() override;
    void Terminate() override;
    void DebugUI() override;
    void Deserialize(const rapidjson::Value& value) override;

    void AddDebugDraw() const;

  private:
    const Engine::TransformComponent* mTransformComponent = nullptr;
    Engine::Math::Vector3 mPosition = Engine::Math::Vector3::Zero;
    Engine::Graphics::Color mColor = Engine::Graphics::Colors::White;

    uint32_t mSlices = 0;
    uint32_t mRings = 0;
    float mRadius = 0.0f;
};
