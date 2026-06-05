#pragma once

#include "CustomTypeIds.h"
#include <Engine/Inc/Engine.h>

#include <string>

namespace Engine::CriticalCore
{
// ---------------------------------------------------------------------------
// CameraShakeService - example-local 2D camera follow + screenshake.
//
// Ported from Critical Core 2's oCamera (objects/oCamera/Create_0.gml,
// Step_0.gml) and scripts/ScreenShake/ScreenShake.gml. It reproduces the
// source's camera behaviour entirely inside the 256x224 internal space:
//
//   * FOLLOW: each fixed step the aim point is
//         aim = floor(lerp(player, oCore, 0.35))
//     i.e. 35% of the way from the player toward the Core (Step_0.gml:5-6).
//     The camera then eases toward the aim with divisor 12:
//         x += (aimX - x) / 12 ;  y += (aimY - y) / 12     (Step_0.gml:26-27)
//
//   * SHAKE: ScreenShake(magnitude, frames) seeds the shake ONLY if the new
//     magnitude is stronger than what remains (ScreenShake.gml:3-7). Each fixed
//     step the residual decays linearly:
//         remain = max(0, remain - magnitude/length)        (Step_0.gml:31)
//     and the view is jittered by random_range(-remain, remain) on BOTH axes
//     (Step_0.gml:30). The jitter is sampled ONCE per Update() (using the
//     pre-decay remain) so GetCameraOffset() is stable within a step and
//     deterministic under a fixed RNG seed.
//
// The exposed offset is the camera TOP-LEFT in 256x224 room coordinates
// (x - internalW/2, y - internalH/2) plus the per-axis shake jitter. It is a
// SCENE-SPACE translation: task 34 subtracts it from world draw origins (or
// translates the Render2D / RenderTarget2D ortho by -offset) BEFORE the point
// upscale. The shake is therefore applied inside 256x224, never to the
// upscaled 768x672 window geometry.
//
// This is an Engine::Service (so task 34 can construct it via
// GameWorld::AddService<CameraShakeService>() through CustomRegistration's
// service table, id CustomServiceId::CameraShakeService) but it stays
// example-local and deliberately does NOT override the Service::Update(float)
// lifecycle: it is driven by the fixed-step GameClock via the no-arg Update()
// below, never by wall-clock dt.
// ---------------------------------------------------------------------------

class CameraShakeService final : public Engine::Service
{
  public:
    SET_TYPE_ID(CustomServiceId::CameraShakeService);

    // Sets the internal resolution and snaps the camera to the room centre
    // (matching oCamera/Create_0.gml: x = room_width/2, y = room_height/2).
    // Resets all shake state. NOTE: this HIDES the no-op Service::Initialize();
    // task 34 calls it explicitly. The base lifecycle Initialize()/Update(float)
    // remain harmless no-ops so the GameWorld auto-lifecycle never fights the
    // fixed-step driver.
    void Initialize(int internalW = 256, int internalH = 224);

    // Per fixed step: the current player and Core positions (room coordinates).
    // Cached for the next Update().
    void SetTargets(float playerX, float playerY, float coreX, float coreY);

    // Runs ONCE per fixed GameClock step (no dt scaling, no wall clock):
    //   1. aim   = floor(lerp(target, core, 0.35))
    //   2. ease  x += (aimX - x)/12 ;  y += (aimY - y)/12
    //   3. jitter = random_range(-remain, remain) per axis (pre-decay remain)
    //   4. decay remain = max(0, remain - magnitude/length)
    void Update();

    // Seeds a shake of 'magnitude' pixels lasting 'frames' fixed steps, but
    // ONLY if 'magnitude' exceeds the residual still in flight (so a bigger hit
    // overrides a fading one, a smaller hit is ignored). ScreenShake.gml:3-7.
    void ScreenShake(float magnitude, int frames);

    // Camera top-left in 256x224 space plus this step's shake jitter. This is
    // the scene translation task 34 consumes (inside 256x224, pre-upscale).
    void GetCameraOffset(float& outX, float& outY) const;

    // Current shake residual (the |jitter| envelope). For selftest/HUD.
    void GetShake(float& outMag) const;

    // Drives ScreenShake(12, 40), steps the service ~45 fixed steps, and asserts
    // the shake envelope peaks at ~12 then monotonically decays to 0 within ~40
    // steps, with the jittered offset bounded by sqrt(2)*envelope each step.
    // Writes a CSV trace to csvOut. Task 35 -> .omo/evidence/task-19-shake.csv.
    bool ShakeSelfTest(std::string& csvOut);

  private:
    int mInternalWidth = 256;
    int mInternalHeight = 224;

    // Camera centre in room coordinates (eased toward the follow aim).
    float mX = 128.0f;
    float mY = 112.0f;

    // Follow targets (player + Core), set per step via SetTargets.
    float mPlayerX = 128.0f;
    float mPlayerY = 112.0f;
    float mCoreX = 128.0f;
    float mCoreY = 112.0f;

    // Shake state (oCamera fields shakeMagnitude/shakeRemain/shakeLength).
    float mShakeMagnitude = 0.0f; // seed magnitude of the active shake
    float mShakeRemain = 0.0f;    // residual (decays to 0); also the jitter bound
    float mShakeLength = 0.0f;    // duration in fixed steps

    // Jitter sampled once per Update() (pre-decay), reused by GetCameraOffset so
    // the offset is stable within a step and deterministic under a seed.
    float mShakeJitterX = 0.0f;
    float mShakeJitterY = 0.0f;
};
} // namespace Engine::CriticalCore
