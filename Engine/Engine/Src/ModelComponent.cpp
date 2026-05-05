#include "Precompiled.h"
#include "ModelComponent.h"
#include "SaveUtil.h"

using namespace Engine;

void ModelComponent::Initialize()
{
}

void ModelComponent::Terminate()
{
    RenderObjectComponent::Terminate();
}

void ModelComponent::Deserialize(const rapidjson::Value& value)
{
}

Graphics::ModelId ModelComponent::GetModelId() const
{
    return Graphics::ModelId();
}

const Graphics::Model& ModelComponent::GetModel() const
{
    static const Graphics::Model sEmpty;
    return sEmpty;
}
