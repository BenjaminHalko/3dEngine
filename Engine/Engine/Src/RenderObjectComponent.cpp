#include "Precompiled.h"
#include "RenderObjectComponent.h"
#include "SaveUtil.h"
#include "GameWorld.h"
#include "GameObject.h"
#include "RenderService.h"

using namespace Engine;

void RenderObjectComponent::Initialize()
{
    RenderService* renderService = GetOwner().GetWorld().GetService<RenderService>();
    if (renderService != nullptr)
    {
        renderService->Register(this);
    }
}

void RenderObjectComponent::Terminate()
{
    RenderService* renderService = GetOwner().GetWorld().GetService<RenderService>();
    if (renderService != nullptr)
    {
        renderService->Unregister(this);
    }
}

void RenderObjectComponent::Deserialize(const rapidjson::Value& value)
{
    SaveUtil::ReadBool("CastShadow", mCastShadow, value);
}

bool RenderObjectComponent::CanCastShadow() const
{
    return mCastShadow;
}
