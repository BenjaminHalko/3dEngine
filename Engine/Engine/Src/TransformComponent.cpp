#include "Precompiled.h"
#include "TransformComponent.h"
#include "SaveUtil.h"

using namespace Engine;
using namespace Engine::Graphics;

void TransformComponent::DebugUI()
{
    ImGui::DragFloat3("Position", &position.x, 0.1f);
    ImGui::DragFloat4("Rotation", &rotation.x, 0.0001f);
    ImGui::DragFloat3("Scale", &scale.x, 0.1f);

    SimpleDraw::AddTransform(GetMatrix4());
}

void TransformComponent::Deserialize(const rapidjson::Value& value)
{
    SaveUtil::ReadVector3("Position", position, value);
    SaveUtil::ReadQuaternion("Rotation", rotation, value);
    SaveUtil::ReadVector3("Scale", scale, value);
}
