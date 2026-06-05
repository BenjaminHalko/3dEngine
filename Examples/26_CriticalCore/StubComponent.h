#pragma once

#include "CustomTypeIds.h"

#include <Engine/Inc/Engine.h>

namespace Engine::CriticalCore
{
// Trivial component that exists only to prove the custom-factory path end-to-end:
// JSON "StubComponent" -> CustomRegistration make callback -> StubComponent::Deserialize.
class StubComponent final : public Engine::Component
{
  public:
    SET_TYPE_ID(CustomComponentId::Stub);

    void Deserialize(const rapidjson::Value& value) override;

    int GetValue() const
    {
        return mValue;
    }

  private:
    int mValue = 0;
};
} // namespace Engine::CriticalCore
