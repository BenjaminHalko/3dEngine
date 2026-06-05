#include "CriticalCore2DRenderService.h"
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
    mRenderTarget.Terminate();
    mRender2D.Terminate();
}

void CriticalCore2DRenderService::Render()
{
    // Ensure GameMaker draw order (depth descending) before painting.
    SortIfNeeded();

    // Render the whole 2D scene into the 256x224 offscreen target.
    mRenderTarget.BeginScene(mClearColor);
    // mRenderables is sorted by depth DESCENDING, so iterating front-to-back of
    // the vector draws the HIGHEST depth first (behind) and the LOWEST depth
    // last (in front) - exactly GameMaker's painter order.
    for (Render2DComponent* renderable : mRenderables)
    {
        renderable->Draw(mRender2D);
    }
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
