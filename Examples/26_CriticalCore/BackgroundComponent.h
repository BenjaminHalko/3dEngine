#pragma once

#include "CustomTypeIds.h"
#include "Render2DComponent.h"

#include <Engine/Inc/Engine.h>

#include <array>
#include <vector>

namespace Engine::CriticalCore
{
// ---------------------------------------------------------------------------
// BackgroundComponent - port of oBackground (objects/oBackground/*.gml).
//
// The animated parallax backdrop: a field of small sBGBubbles sprites that drift
// slowly outward from the arena centre, fade in, gently wobble (a per-bubble sine
// "wave" offset), and respawn near the centre once they drift past the room edge.
// (oBackground/Step_0.gml + Draw_0.gml.)
//
// bgFlash: a 0..1 flash value driven by core damage (CoreFunctions.gml sets
// oBackground.bgFlash = 1). In the source it tints the dark octagon backdrop
// (merge_color(#00001a,#0a0a60,bgFlash)). That filled-polygon backdrop is NOT
// reproduced here (Render2D has no filled-polygon primitive - circles/lines/
// sprites/text only); the RT clears to black instead. bgFlash is still tracked
// and decays each step (Approach(bgFlash,0,0.2)) so callers have a working hook,
// and it is exposed for whoever wires the octagon fill later.
//
// PLACEMENT: the level (task 33) places exactly one background object; it is NOT
// spawned at runtime. The level JSON sets the TransformComponent + Depth (drawn
// behind everything: GameMaker depth semantics => a high Depth value).
//
// CORE-DAMAGE HOOK (CoreFunctions.gml DamageCore => oBackground.bgFlash = 1):
//   auto* bg = ...->GetComponent<BackgroundComponent>();
//   bg->SetFlash(1.0f);   // call when the core takes a fireball hit
// ---------------------------------------------------------------------------

class BackgroundComponent final : public Render2DComponent
{
  public:
    SET_TYPE_ID(CustomComponentId::BackgroundComponent);

    void Initialize() override;
    void Update(float deltaTime) override;
    void Draw(Render2D& render2D) override;

    void Deserialize(const rapidjson::Value& value) override;

    // Core-damage flash (0..1). CoreFunctions.gml sets this to 1 on a hit; it
    // decays back to 0 each step.
    void SetFlash(float flash)
    {
        mBgFlash = flash;
    }
    float GetFlash() const
    {
        return mBgFlash;
    }

  private:
    static constexpr int kFrameCount = 4; // sprite_get_number(sBGBubbles) == 4

    // One drifting backdrop bubble (oBackground bubbles[] struct).
    struct Bubble
    {
        float x = 0.0f;
        float y = 0.0f;
        float dir = 0.0f;    // heading from centre (degrees)
        float spd = 0.0f;    // drift speed (eased toward 0.15)
        float alpha = 0.0f;  // eased toward 0.5
        float purple = 0.0f; // weapon-bubble tint amount (0 here; no oBubble link)
        int index = 0;       // sBGBubbles frame
    };

    void SpawnBubbles();

    std::array<Graphics::TextureId, kFrameCount> mFrames{};
    bool mLoaded = false;

    std::vector<Bubble> mBubbles;
    int mBubbleCount = 200; // oBackground.bubbleCount

    float mBgFlash = 0.0f;
    float mTime = 0.0f; // accumulated fixed-step seconds, drives the Wave wobble
};
} // namespace Engine::CriticalCore
