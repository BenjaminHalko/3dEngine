#pragma once

#include "Collision.h"
#include "Render2DComponent.h"

#include <Engine/Inc/Engine.h>

namespace Engine::CriticalCore
{
class Render2D;
class BeatService;

// ---------------------------------------------------------------------------
// WallComponent - the VISUAL + beat/menu state for an arena wall (port of
// objects/oWall). Derives from Render2DComponent so the CriticalCore2DRenderService
// collects and depth-orders it with every other 2D drawable.
//
// SINGLE-SOURCE-OF-TRUTH GEOMETRY: this component does NOT define wall positions
// or angles. It consumes the canonical octagon geometry from Collision (task 17):
//   * Outer walls (index 0..7): the baseline xstart/ystart/angle/xscaleStart are
//     looked up from Collision::OuterWalls() by `index`.
//   * Boss walls (index -1): centered at the arena center, angle/scale supplied at
//     runtime by the Core (the Core spawns boss walls and grows their length).
// The Core's filled VISUAL octagon (oCore.polygonPoints) is a separate primitive;
// walls only render their own segment.
//
// BEAT / COLOR PULSE (oWall/Step_0.gml:3-8 + oMusicController/Step_0.gml:21-49):
//   * beatPulse / colorPulse decay each fixed step (ApproachFade).
//   * On BeatService::AudioTick() the per-beat wall-index selection (ported from
//     oMusicController) decides whether THIS wall pulses (beatPulse/colorPulse = 1),
//     plus the on-whole-beat general boost. Boss walls (index -1) get a larger boost.
//   * image_blend = merge_color(white, BeatService::WallPulseColor(), colorPulse).
//     Boss walls additionally tint toward the pulse color by their heal fraction
//     (hpWaitHeal / coreWaitToHeal()), supplied by the Core via SetBossHealFraction.
//
// MENU-STATE OUTWARD SCALE (oWall/Step_0.gml:10-16, outer walls only):
//   scaleMenu eases toward the "expanded" target; _scale = lerp(1,2,scaleMenu) so
//   the wall lengthens (image_xscale *= _scale) and its origin moves along
//   lerp(arena_center, xstart, _scale). On the title menu _scale settles at 1 (the
//   canonical Collision position) and on game start it eases out to 2, so the
//   octagon visibly EXPANDS outward when a game begins. The expand target maps to
//   the GML `!instance_exists(oMenu) and !oLeaderboardAPI.draw`, read directly from
//   MenuComponent::IsMenuActive() (expand == !IsMenuActive()). SetMenuExpand remains
//   an optional external override that can also force the expand.
// ---------------------------------------------------------------------------

class WallComponent final : public Render2DComponent
{
  public:
    SET_TYPE_ID(CustomComponentId::WallComponent);

    void Initialize() override;
    void Terminate() override;
    void Update(float deltaTime) override; // one call == one fixed step (GameClock::kStep)
    void DebugUI() override;

    void Deserialize(const rapidjson::Value& value) override;

    void Draw(Render2D& render2D) override;

    // --- Cross-component drive points (caller-supplied state, keeps WallComponent
    //     decoupled from CoreComponent / MenuComponent which live in other files) ---

    // Outer-wall menu/title outward-scale target. Maps to oWall's GML target
    // `!instance_exists(oMenu) and !oLeaderboardAPI.draw`. true => expand toward 2x.
    void SetMenuExpand(bool expand)
    {
        mMenuExpand = expand;
    }

    // Boss-wall runtime length (image_xscale). The Core grows this each step
    // (oCore boss-wall build/scale). Only meaningful when bossWall == true.
    void SetBossScale(float imageXscale)
    {
        mBossScaleX = imageXscale;
    }

    // Boss-wall heal tint fraction = oCore.hpWaitHeal / coreWaitToHeal() in [0,1],
    // supplied by the Core (oWall/Step_0.gml:6-8). Only used when bossWall == true.
    void SetBossHealFraction(float fraction)
    {
        mBossHealFraction = fraction;
    }

    int GetIndex() const
    {
        return mIndex;
    }
    bool IsBossWall() const
    {
        return mBossWall;
    }

  private:
    // Per-step beat-pulse selection ported from oMusicController/Step_0.gml:21-49.
    void ApplyBeatPulse(const BeatService& beat);

    // --- Authored / canonical state ---
    int mIndex = 0;          // image_angle div 45 (octant); -1 for boss walls.
    bool mFlipped = false;   // oWall.flipped (boss walls spawn flipped == true).
    bool mBossWall = false;  // oWall.bossWall.
    float mAngle = 0.0f;     // image_angle (deg) — canonical from Collision unless overridden.
    float mScaleXStart = 0.0f; // xscaleStart (image_xscale baseline). length = 4 * this.
    float mXStart = Arena::kCenterX; // oWall.xstart (creation x; canonical from Collision).
    float mYStart = Arena::kCenterY; // oWall.ystart (creation y; canonical from Collision).

    // --- Live per-step state (oWall instance vars) ---
    float mBeatPulse = 0.0f;  // thickness pulse, decays to 0.
    float mColorPulse = 0.0f; // color-blend pulse, decays to 0.
    float mScaleMenu = 0.0f;  // eased menu-expand factor in [0,1].
    Engine::Graphics::Color mImageBlend = Engine::Graphics::Colors::White;

    // --- Caller-driven cross-component inputs ---
    bool mMenuExpand = false;       // SetMenuExpand target.
    float mBossScaleX = 0.0f;       // SetBossScale runtime length.
    float mBossHealFraction = 0.0f; // SetBossHealFraction heal tint.

    BeatService* mBeatService = nullptr;
};
} // namespace Engine::CriticalCore
