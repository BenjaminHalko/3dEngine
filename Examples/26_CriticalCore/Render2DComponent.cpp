#include "Render2DComponent.h"
#include "CriticalCore2DRenderService.h"

using namespace Engine;

namespace Engine::CriticalCore
{
void Render2DComponent::Initialize()
{
    // Position source: the owning GameObject's TransformComponent (may be null
    // for transform-less objects; GetWorldX/Y then report 0).
    mTransformComponent = GetOwner().GetComponent<TransformComponent>();

    // Register with the example-local 2D render service (mirrors the engine's
    // RenderObjectComponent -> RenderService register pattern).
    mRenderService = GetOwner().GetWorld().GetService<CriticalCore2DRenderService>();
    if (mRenderService != nullptr)
    {
        mRenderService->Register(this);
    }
}

void Render2DComponent::Terminate()
{
    if (mRenderService != nullptr)
    {
        mRenderService->Unregister(this);
        mRenderService = nullptr;
    }
    mTransformComponent = nullptr;
}

void Render2DComponent::Deserialize(const rapidjson::Value& value)
{
    SaveUtil::ReadFloat("Depth", mDepth, value);
}

float Render2DComponent::GetWorldX() const
{
    return (mTransformComponent != nullptr) ? mTransformComponent->position.x : 0.0f;
}

float Render2DComponent::GetWorldY() const
{
    return (mTransformComponent != nullptr) ? mTransformComponent->position.y : 0.0f;
}

void Render2DComponent::GetDrawPosition(float& outX, float& outY) const
{
    outX = GetWorldX();
    outY = GetWorldY();
    ApplyCameraOffset(outX, outY);
}

void Render2DComponent::ApplyCameraOffset(float& x, float& y) const
{
    if (mRenderService != nullptr)
    {
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        mRenderService->GetCameraOffset(offsetX, offsetY);
        x -= offsetX;
        y -= offsetY;
    }
}
} // namespace Engine::CriticalCore
