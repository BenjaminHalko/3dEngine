#include "WallComponent.h"

#include "Collision.h"
#include "GmHelpers.h"
#include "MenuComponent.h"
#include "MusicController.h"
#include "Render2D.h"

#include <algorithm>
#include <cmath>

using namespace Engine;

namespace Engine::CriticalCore
{
namespace
{
// Whole-beat predicate: audioBeat is a multiple of 0.5, so it lands on a whole
// beat when fmod(audioBeat, 1) == 0 (oMusicController/Step_0.gml:25,30,44).
bool OnWholeBeat(float audioBeat)
{
    return std::fmod(audioBeat, 1.0f) == 0.0f;
}
} // namespace

void WallComponent::Initialize()
{
    // Base: cache TransformComponent + register with CriticalCore2DRenderService.
    Render2DComponent::Initialize();

    // Resolve the shared beat clock (added pre-Initialize). May be null if the
    // level omitted it — the wall then just draws white with no pulse.
    mBeatService = GetOwner().GetWorld().GetService<BeatService>();

    // CANONICAL GEOMETRY (single source of truth = Collision/task 17). Outer walls
    // look up their baseline xstart/ystart/angle/xscaleStart from OuterWalls() by
    // index; JSON Angle/Scale (if present) only override these. Boss walls live at
    // the arena center and take their angle from JSON, length from SetBossScale.
    if (!mBossWall)
    {
        for (const WallSegment& w : OuterWalls())
        {
            if (w.index == mIndex)
            {
                mXStart = w.x;
                mYStart = w.y;
                if (mAngle == 0.0f) // not overridden in JSON
                {
                    mAngle = w.angle;
                }
                if (mScaleXStart == 0.0f) // not overridden in JSON
                {
                    mScaleXStart = w.length / Arena::kWallSpriteThickness; // length / 4
                }
                break;
            }
        }
    }
    else
    {
        // Boss walls spawn at the core center (oCore/Create_0.gml:44).
        mXStart = Arena::kCenterX;
        mYStart = Arena::kCenterY;
    }

    // oWall/Create_0.gml initial state.
    mBeatPulse = 0.0f;
    mColorPulse = 0.0f;
    mScaleMenu = 0.0f;
    mImageBlend = Engine::Graphics::Colors::White;
}

void WallComponent::Terminate()
{
    mBeatService = nullptr;
    Render2DComponent::Terminate();
}

void WallComponent::ApplyBeatPulse(const BeatService& beat)
{
    // oMusicController/Step_0.gml:21-49 — runs on the rising half-beat tick.
    const float audioBeat = beat.AudioBeat();
    const int beatInt = static_cast<int>(audioBeat);
    const int pulseType = beat.WallPulseType();

    // Per-beat wall-index selection (:22-34): build the 2-element index set and
    // pulse this wall to full if its index is contained.
    if (OnWholeBeat(audioBeat))
    {
        int sel0 = 0;
        int sel1 = 0;
        if (pulseType == 1)
        {
            // case 1: [7 - beat, 7 - (4 + beat) % 8]
            sel0 = 7 - beatInt;
            sel1 = 7 - ((4 + beatInt) % 8);
        }
        else
        {
            // default/case 0: [beat, (4 + beat) % 8]
            sel0 = beatInt;
            sel1 = (4 + beatInt) % 8;
        }
        if (mIndex == sel0 || mIndex == sel1)
        {
            mBeatPulse = 1.0f;
            mColorPulse = 1.0f;
        }
    }

    // General on-whole-beat boost (:44-49). Boss walls (index == -1) pulse harder.
    if (OnWholeBeat(audioBeat))
    {
        const float isBoss = (mIndex == -1) ? 1.0f : 0.0f;
        mBeatPulse = std::max(mBeatPulse, 0.5f + 2.0f * isBoss);
        mColorPulse = std::max(mColorPulse, 0.5f + 0.2f * isBoss);
    }
}

void WallComponent::Update(float deltaTime)
{
    (void)deltaTime; // logic is fixed-step (GameClock); never scaled by dt.

    // oWall/Step_0.gml:3-4 — pulses decay each step.
    mBeatPulse = ApproachFade(mBeatPulse, 0.0f, 0.05f, 0.7f);
    mColorPulse = ApproachFade(mColorPulse, 0.0f, 0.02f, 0.7f);

    // On the half-beat tick, run the per-wall pulse selection (re-boosts above).
    if (mBeatService != nullptr && mBeatService->AudioTick())
    {
        ApplyBeatPulse(*mBeatService);
    }

    // oWall/Step_0.gml:5 — image_blend = merge_color(white, wallPulseColor, colorPulse).
    const Color pulseColor =
        (mBeatService != nullptr) ? mBeatService->WallPulseColor() : Engine::Graphics::Colors::White;
    mImageBlend = MergeColor(Engine::Graphics::Colors::White, pulseColor, mColorPulse);

    // oWall/Step_0.gml:6-8 — boss walls additionally tint toward the pulse color by
    // their heal fraction (oCore.hpWaitHeal / coreWaitToHeal()), pushed by the Core.
    if (mBossWall)
    {
        mImageBlend = MergeColor(mImageBlend, pulseColor, mBossHealFraction);
    }
    else
    {
        // oWall/Step_0.gml:10-11 — ease the menu-expand factor toward its target.
        // GML target `!instance_exists(oMenu) and !oLeaderboardAPI.draw` is TRUE
        // only once a game runs; it maps to !IsMenuActive() (==!(sMenuActive &&
        // !sInGame)). SetMenuExpand stays as an optional external override.
        const bool inGame = !MenuComponent::IsMenuActive();
        const float target = (inGame || mMenuExpand) ? 1.0f : 0.0f;
        mScaleMenu = ApproachFade(mScaleMenu, target, 0.04f, 0.8f);
    }
}

void WallComponent::Draw(Render2D& render2D)
{
    // Resolve the live instance transform for this step.
    float originX = mXStart;
    float originY = mYStart;
    float imageXscale = mScaleXStart;

    if (mBossWall)
    {
        // Boss walls stay centered; the Core grows their length (image_xscale).
        originX = Arena::kCenterX;
        originY = Arena::kCenterY;
        imageXscale = mBossScaleX;
    }
    else
    {
        // oWall/Step_0.gml:12-15 — menu-state outward scale (1 -> 2).
        const float scale = Math::Lerp(1.0f, 2.0f, mScaleMenu);
        imageXscale = mScaleXStart * scale;
        originX = Math::Lerp(Arena::kCenterX, mXStart, scale);
        originY = Math::Lerp(Arena::kCenterY, mYStart, scale);
    }

    // Build the segment through Collision's canonical centerline math so the visual
    // matches the collision geometry exactly (single source of truth, task 17).
    const WallSegment seg = MakeWall(originX, originY, mAngle, imageXscale, mFlipped, mBossWall);

    // Fold in the render service's camera follow + shake offset.
    float ax = seg.ax;
    float ay = seg.ay;
    float bx = seg.bx;
    float by = seg.by;
    ApplyCameraOffset(ax, ay);
    ApplyCameraOffset(bx, by);

    // oWall/Draw_0.gml: y-scale = min(1, beatPulse) + 1 over the 4px sprite, i.e.
    // the drawn thickness pulses from 4 (rest) to 8 (full beat).
    const float thickness = Arena::kWallSpriteThickness * (std::min(1.0f, mBeatPulse) + 1.0f);

    // GM image_blend is an RGB-only colour; the wall's image_alpha is always 1.
    // Our MergeColor lerps the alpha channel too, so force it back to 1 here (else
    // a low-alpha WallPulseColor would fade the wall to invisible).
    Color blend = mImageBlend;
    blend.a = 1.0f;
    render2D.DrawLine(ax, ay, bx, by, thickness, blend);
}

void WallComponent::DebugUI()
{
    ImGui::Text("Wall idx=%d %s", mIndex, mBossWall ? "(boss)" : "");
    ImGui::Text("beatPulse=%.2f colorPulse=%.2f", mBeatPulse, mColorPulse);
    ImGui::Text("scaleMenu=%.2f inGame=%d expand=%d", mScaleMenu,
        MenuComponent::IsMenuActive() ? 0 : 1, mMenuExpand ? 1 : 0);
}

void WallComponent::Deserialize(const rapidjson::Value& value)
{
    // Base reads "Depth".
    Render2DComponent::Deserialize(value);

    SaveUtil::ReadInt("Index", mIndex, value);
    SaveUtil::ReadBool("Flipped", mFlipped, value);
    SaveUtil::ReadBool("BossWall", mBossWall, value);
    // Angle / Scale are OPTIONAL overrides; left at 0 they inherit the canonical
    // Collision geometry for this index in Initialize().
    SaveUtil::ReadFloat("Angle", mAngle, value);
    SaveUtil::ReadFloat("Scale", mScaleXStart, value);
}
} // namespace Engine::CriticalCore
