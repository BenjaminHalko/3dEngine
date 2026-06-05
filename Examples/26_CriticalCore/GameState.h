#pragma once

#include <Engine/Inc/Engine.h>

#include "GameClock.h"
#include "GameFlow.h"

#include <filesystem>

namespace Engine::CriticalCore
{
class CriticalCore2DRenderService;
class CameraShakeService;
class BeatService;
class GuiComponent;
} // namespace Engine::CriticalCore

// ---------------------------------------------------------------------------
// GameState - the integration seam for "Critical Core 2".
//
// Wires the engine GameWorld + the example-local custom component/service factory
// (RegisterCriticalCoreTypes) to the CriticalCore.json level, owns the GameFlow
// state machine, drives the fixed 60Hz GameClock (GameFlow + GameWorld + camera +
// camera-offset push), and renders the 256x224 scene into the RenderTarget which
// the CriticalCore2DRenderService upscales to the 768x672 window.
//
// All gameplay updates run INSIDE the fixed GameClock step; rendering is locked to
// ticks (no interpolation), and the camera shake offset is pushed into the render
// service every step. No gameplay is drawn to the backbuffer directly - the RT +
// point upscale owns the backbuffer.
// ---------------------------------------------------------------------------
class GameState : public Engine::AppState
{
  public:
    void Initialize() override;
    void Terminate() override;
    void Update(float deltaTime) override;
    void Render() override;
    void DebugUI() override;

  private:
    // Load the level + (re)wire the flow, services, camera, and HUD. Used by
    // Initialize() and the dev reload path.
    void LoadAndWire();
    // Tear down the flow + world (without clearing the static factory callbacks)
    // then LoadAndWire() again - a clean restart back to the title/menu.
    void ReloadLevel();

    std::filesystem::path mLevelFile;
    Engine::GameWorld mGameWorld;

    Engine::CriticalCore::GameClock mClock;
    Engine::CriticalCore::GameFlow mGameFlow;

    // Resolved from the world after LoadLevel (refreshed on reload).
    Engine::CriticalCore::CriticalCore2DRenderService* mRenderService = nullptr;
    Engine::CriticalCore::CameraShakeService* mCameraShake = nullptr;
    Engine::CriticalCore::BeatService* mBeatService = nullptr;
    Engine::CriticalCore::GuiComponent* mGui = nullptr;
};
