#pragma once

#include "CustomTypeIds.h"

#include <Engine/Inc/Engine.h>

namespace Engine::CriticalCore
{
class Render2D;
class CriticalCore2DRenderService;

// ---------------------------------------------------------------------------
// Render2DComponent - base renderable for the example-local 2D pipeline.
//
// This is the GameObject-side counterpart to CriticalCore2DRenderService. Every
// drawable entity in Critical Core 2 (Core, Player, Bubble, Fireball, Spike,
// Wall, Trail, Sparkle, Background, GUI, ...) derives its rendering component
// from this base so that the service can collect, depth-sort, and draw them all
// into the 256x224 RenderTarget2D each frame. Tasks 20-29 subclass this.
//
// Lifecycle (mirrors the engine RenderObjectComponent pattern,
// Engine/Engine/Src/RenderObjectComponent.cpp):
//   * Initialize(): caches the owning GameObject's TransformComponent (the
//     position source) and registers `this` with CriticalCore2DRenderService.
//   * Terminate():  unregisters `this` from the service.
//
// Drawing:
//   * Override Draw(Render2D&) to emit sprites/primitives/text. The default is
//     a no-op (an entity may exist purely for logic with no visual).
//   * Subclasses draw in WORLD space (256x224 room coordinates). To honour the
//     CameraShakeService follow/shake offset stored on the service (task 34
//     pushes it each fixed step via CriticalCore2DRenderService::SetCameraOffset)
//     subclasses call GetDrawPosition()/ApplyCameraOffset() which subtract the
//     service's current (offsetX, offsetY) from the world coordinate. This is
//     how "the service applies the offset": the offset lives on the service and
//     the base component folds it into the draw origin so subclasses never have
//     to know about the camera.
//
// Depth:
//   * `depth` is the GameMaker draw-order field. GameMaker semantics: objects
//     with a HIGHER depth are drawn FIRST (further behind); LOWER depth draws
//     last (on top / in front). The service keeps its registry sorted by depth
//     descending and iterates front-to-back of that vector (i.e. highest depth
//     first) so the painter's order matches GameMaker exactly.
//   * Deserialize reads the JSON "Depth" key (float; default 0).
// ---------------------------------------------------------------------------

class Render2DComponent : public Engine::Component
{
  public:
    SET_TYPE_ID(CustomComponentId::Render2DComponent);

    void Initialize() override;
    void Terminate() override;

    void Deserialize(const rapidjson::Value& value) override;

    // Emit this entity's draws into the 2D layer. World-space (256x224); the
    // base helpers below fold in the camera offset. Default: nothing to draw.
    virtual void Draw(Render2D& render2D)
    {
    }

    // GameMaker depth: higher = behind, lower = in front. Used by the service
    // to order the registry (descending) before drawing.
    float GetDepth() const
    {
        return mDepth;
    }

    void SetDepth(float depth)
    {
        mDepth = depth;
    }

  protected:
    // Raw world position from the owning GameObject's TransformComponent
    // (z = 0 plane; .x/.y only). Zero if the object has no transform.
    float GetWorldX() const;
    float GetWorldY() const;

    // World position with the service's current camera offset already applied
    // (worldXY - cameraOffsetXY). This is the coordinate subclasses should hand
    // to Render2D so the whole scene tracks the camera follow + shake.
    void GetDrawPosition(float& outX, float& outY) const;

    // Translate an arbitrary world coordinate into draw space (subtract the
    // service camera offset). For draws not anchored at the transform origin.
    void ApplyCameraOffset(float& x, float& y) const;

    const TransformComponent* GetTransformComponent() const
    {
        return mTransformComponent;
    }

    CriticalCore2DRenderService* GetRenderService() const
    {
        return mRenderService;
    }

  private:
    const TransformComponent* mTransformComponent = nullptr;
    CriticalCore2DRenderService* mRenderService = nullptr;

    float mDepth = 0.0f;
};
} // namespace Engine::CriticalCore
