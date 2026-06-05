#include "StubComponent.h"

namespace Engine::CriticalCore
{
void StubComponent::Deserialize(const rapidjson::Value& value)
{
    if (value.HasMember("value"))
    {
        mValue = value["value"].GetInt();
    }
}
} // namespace Engine::CriticalCore
