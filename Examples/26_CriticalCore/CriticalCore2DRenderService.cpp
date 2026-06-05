#include "CriticalCore2DRenderService.h"
#include "CameraShakeService.h"
#include "Render2DComponent.h"

#include <algorithm>

using namespace Engine;
using namespace Engine::Graphics;

namespace Engine::CriticalCore
{
void CriticalCore2DRenderService::Initialize()
{
    mRender2D.Initialize();
    mRenderTarget.Initialize();
}

void CriticalCore2DRenderService::Terminate()
{
    mRenderables.clear();
    SafeRelease(mAlphaBlendState);
    mRenderTarget.Terminate();
    mRender2D.Terminate();
}

void CriticalCore2DRenderService::EnsureAlphaBlendState()
{
    if (mAlphaBlendState != nullptr)
    {
        return;
    }

    D3D11_BLEND_DESC desc{};
    desc.RenderTarget[0].BlendEnable = TRUE;
    desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    auto device = GraphicsSystem::Get()->GetDevice();
    device->CreateBlendState(&desc, &mAlphaBlendState);
}

void CriticalCore2DRenderService::Render()
{
    // Ensure GameMaker draw order (depth descending) before painting.
    SortIfNeeded();
    EnsureAlphaBlendState();

    // Fold the camera's dynamic zoom (oCamera.scale) into the 2D view so every
    // draw this frame tracks it. Read from the static mirror so the render
    // service needs no CameraShakeService pointer.
    mRender2D.SetViewScale(CameraShakeService::ViewScale());

    auto context = GraphicsSystem::Get()->GetContext();

    // Render the whole 2D scene into the 256x224 offscreen target.
    mRenderTarget.BeginScene(mClearColor);
    context->OMSetBlendState(mAlphaBlendState, nullptr, 0xffffffffu);
    // mRenderables is sorted by depth DESCENDING, so iterating front-to-back of
    // the vector draws the HIGHEST depth first (behind) and the LOWEST depth
    // last (in front) - exactly GameMaker's painter order.
    for (Render2DComponent* renderable : mRenderables)
    {
        renderable->Draw(mRender2D);
    }
    context->OMSetBlendState(nullptr, nullptr, 0xffffffffu);
    mRenderTarget.EndScene();

    // Letterboxed POINT upscale to the backbuffer.
    GraphicsSystem* graphicsSystem = GraphicsSystem::Get();
    const int windowWidth = static_cast<int>(graphicsSystem->GetBackBufferWidth());
    const int windowHeight = static_cast<int>(graphicsSystem->GetBackBufferHeight());
    mRenderTarget.Present(windowWidth, windowHeight);
}

void CriticalCore2DRenderService::Register(Render2DComponent* renderable)
{
    if (renderable == nullptr)
    {
        return;
    }
    auto iter = std::find(mRenderables.begin(), mRenderables.end(), renderable);
    if (iter == mRenderables.end())
    {
        mRenderables.push_back(renderable);
        mSortDirty = true;
    }
}

void CriticalCore2DRenderService::Unregister(Render2DComponent* renderable)
{
    auto iter = std::find(mRenderables.begin(), mRenderables.end(), renderable);
    if (iter != mRenderables.end())
    {
        mRenderables.erase(iter);
        mSortDirty = true;
    }
}

void CriticalCore2DRenderService::SortIfNeeded()
{
    if (!mSortDirty)
    {
        return;
    }
    // Descending by depth: higher depth first (drawn behind), lower depth last
    // (drawn in front). std::stable_sort keeps insertion order among equal
    // depths so same-layer objects draw in registration order (GameMaker ties
    // resolve by instance creation order, which our registration mirrors).
    std::stable_sort(mRenderables.begin(),
                     mRenderables.end(),
                     [](const Render2DComponent* lhs, const Render2DComponent* rhs)
                     { return lhs->GetDepth() > rhs->GetDepth(); });
    mSortDirty = false;
}
} // namespace Engine::CriticalCore
