#pragma once

#include "CustomTypeIds.h"
#include "Render2D.h"
#include "RenderTarget2D.h"

#include <Engine/Inc/Engine.h>

#include <vector>

namespace Engine::CriticalCore
{
class Render2DComponent;

// ---------------------------------------------------------------------------
// CriticalCore2DRenderService - the example-local 2D render service.
//
// This is the glue that ties the three Critical Core 2 rendering pieces to the
// engine's GameWorld service model:
//   * Render2D       - the y-DOWN 256x224 sprite/primitive/text draw layer.
//   * RenderTarget2D - the offscreen 256x224 target + point-sampled letterbox
//                      upscale to the 768x672 backbuffer.
//   * GameWorld      - the Engine::Service lifecycle + per-frame Render() hook.
//
// It owns one Render2D and one RenderTarget2D (Initialize/Terminate them), keeps
// a registry of Render2DComponent renderables, and each frame renders them all
// into the RT then upscales to the window. It deliberately does NOT use the
// built-in Engine::RenderService / UIRenderService: the RT + point upscale owns
// the backbuffer for this example, so 3D render services would fight it.
//
// DRAW ORDER (GameMaker depth semantics):
//   GameMaker draws objects with a HIGHER `depth` FIRST (further behind) and
//   LOWER `depth` LAST (on top / in front). The registry is therefore kept
//   sorted by depth DESCENDING and iterated in that order, so the painter's
//   algorithm reproduces the original layering exactly. The list is re-sorted
//   lazily (dirty flag) on Register/Unregister/SetDepth, just before Render().
//
// CAMERA OFFSET (task 19 CameraShakeService -> task 34 wiring):
//   The service stores a single (offsetX, offsetY) = the camera top-left in
//   256x224 room space plus screenshake jitter. Each fixed step task 34 reads
//   CameraShakeService::GetCameraOffset(...) and pushes it here via
//   SetCameraOffset(...). Components draw in world space and fold the offset in
//   through Render2DComponent::GetDrawPosition()/ApplyCameraOffset() (which
//   query this service), so the whole scene tracks the camera follow + shake
//   inside the 256x224 RT, before the point upscale.
//
// RENDER() SEQUENCE (called from GameState::Render via mGameWorld.Render(),
// wired by task 34):
//   1. sort the registry if dirty (depth descending)
//   2. mRenderTarget.BeginScene(clearColor)         // bind + clear the 256x224 RT
//   3. for each component (highest depth -> lowest): comp->Draw(mRender2D)
//   4. mRenderTarget.EndScene()                      // restore prev target/viewport
//   5. mRenderTarget.Present(backBufferW, backBufferH)  // letterboxed POINT upscale
//      (window size from Graphics::GraphicsSystem::Get()->GetBackBufferWidth/Height)
//
// Constructed via GameWorld::AddService<CriticalCore2DRenderService>() through
// CustomRegistration's service table (id CustomServiceId::CriticalCore2DRenderService).
// ---------------------------------------------------------------------------

class CriticalCore2DRenderService final : public Engine::Service
{
  public:
    SET_TYPE_ID(CustomServiceId::CriticalCore2DRenderService);

    // Owns + initializes the Render2D layer and the 256x224 RenderTarget2D.
    void Initialize() override;
    void Terminate() override;

    // Draws all registered renderables into the RT then upscales to the window.
    void Render() override;

    // Registry management. Register keeps no duplicates; both mark the list
    // dirty so Render() re-sorts by depth before drawing.
    void Register(Render2DComponent* renderable);
    void Unregister(Render2DComponent* renderable);

    // Mark the registry as needing a depth re-sort (call after mutating a
    // component's depth at runtime). The sort happens lazily in Render().
    void MarkDepthDirty()
    {
        mSortDirty = true;
    }

    // Scene clear color for the RT (default black). Set before Render() if a
    // different backdrop is wanted under the drawn scene.
    void SetClearColor(const Graphics::Color& clearColor)
    {
        mClearColor = clearColor;
    }

    // Camera offset (256x224 room-space top-left + shake jitter). Task 34 pushes
    // CameraShakeService::GetCameraOffset(...) here each fixed step.
    void SetCameraOffset(float offsetX, float offsetY)
    {
        mCameraOffsetX = offsetX;
        mCameraOffsetY = offsetY;
    }
    void GetCameraOffset(float& outX, float& outY) const
    {
        outX = mCameraOffsetX;
        outY = mCameraOffsetY;
    }

    // Accessors so components / external passes can draw through the same layer
    // and target this service owns.
    Render2D& GetRender2D()
    {
        return mRender2D;
    }
    RenderTarget2D& GetRenderTarget()
    {
        return mRenderTarget;
    }

  private:
    // Re-sorts mRenderables by depth descending (higher depth first) if dirty.
    void SortIfNeeded();

    // Lazily creates mAlphaBlendState on first Render().
    void EnsureAlphaBlendState();

    // Lazily creates mDepthDisabledState on first Render().
    void EnsureDepthDisabledState();

    // Paints the black void around the octagonal arena hole (oBackground mask:
    // inside the octagon = navy backdrop + game, outside = solid black). Drawn in
    // world space at the given zoom so the hole tracks the arena; injected
    // between the world layer and the screen-space HUD (see kVoidMaskDepth).
    void DrawVoidMask(float worldViewScale);

    // GameMaker depth at which the void mask is painted: ABOVE the navy bubbles
    // (depth 50) so they are clipped to the octagon, BELOW the HUD (score 20 /
    // gui 10 / menu 0) so the UI stays on top of the void.
    static constexpr float kVoidMaskDepth = 30.0f;

    Render2D mRender2D;
    RenderTarget2D mRenderTarget;

    // Blend-only SrcAlpha/InvSrcAlpha state for the in-RT 2D scene. NOT the
    // engine Graphics::BlendState: that couples a NOT_EQUAL depth state which
    // would reject the coplanar (z=0) painter's-order overdraw of this scene.
    ID3D11BlendState* mAlphaBlendState = nullptr;

    // Depth test/write DISABLED: the 2D scene is coplanar (z=0) and ordered by
    // the painter's algorithm (depth-sorted registry). With the default LESS
    // depth state the RenderTarget's attached depth buffer would reject every
    // later same-z overdraw (outline behind its fill, bubbles over the player).
    ID3D11DepthStencilState* mDepthDisabledState = nullptr;

    // Registry of renderables, kept sorted by depth DESCENDING (GameMaker order:
    // higher depth drawn first / behind, lower depth drawn last / in front).
    std::vector<Render2DComponent*> mRenderables;
    bool mSortDirty = false;

    // Navy backdrop #00001a = RGB(0,0,26) (oBackground/Draw_0.gml:13 merge_color base).
    Graphics::Color mClearColor = Graphics::Color{0.0f, 0.0f, 26.0f / 255.0f, 1.0f};

    // Camera offset applied to the 2D scene (see header notes).
    float mCameraOffsetX = 0.0f;
    float mCameraOffsetY = 0.0f;
};
} // namespace Engine::CriticalCore
