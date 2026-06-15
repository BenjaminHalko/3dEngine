#include "GameState.h"

#include "CameraShakeService.h"
#include "CoreComponent.h"
#include "CriticalCore2DRenderService.h"
#include "CustomRegistration.h"
#include "GuiComponent.h"
#include "MenuComponent.h"
#include "MusicController.h" // BeatService
#include "PlayerComponent.h"

using namespace Engine;
using namespace Engine::CriticalCore;

namespace
{
// 256x224 internal arena centre - the camera-follow fallback when no Core/Player
// is live yet (title/menu, or the brief window between PlayerExplode and Respawn).
constexpr float kArenaCenterX = 128.0f;
constexpr float kArenaCenterY = 112.0f;
} // namespace

void GameState::Initialize()
{
    mLevelFile = L"Assets/Templates/Levels/CriticalCore.json";

    // Critical Core 2 is a GAME, not a tech demo: ESC must return to the title (a
    // ReloadLevel below), NOT close the whole application. Opt out of the App's
    // global ESC-quit (App.cpp), which otherwise fires before this state's Update()
    // ever sees the key. Other examples keep the default quit-on-ESC convenience.
    Engine::MainApp().SetQuitOnEscape(false);

    // Install the custom component + service factory callbacks ONCE, BEFORE any
    // LoadLevel so GameWorld can resolve the level's unknown component/service
    // names. The callbacks are static and survive reloads.
    RegisterCriticalCoreTypes(mGameWorld);

    LoadAndWire();
}

void GameState::LoadAndWire()
{
    // The HUD is level-placed (gui.json, Depth -1000). Publish the flow's shared
    // GuiState through the static bridge BEFORE LoadLevel, because LoadLevel
    // Initialize()s the Gui object immediately (GameWorld.cpp:204) and
    // GuiComponent::Initialize() reads the static then. GetGuiState() is a stable
    // member pointer, valid before GameFlow::Initialize() populates its values.
    GuiComponent::SetActiveState(mGameFlow.GetGuiState());

    // LoadLevel ONLY (it calls Initialize(capacity) from the JSON "Capacity"
    // internally - calling GameWorld::Initialize() as well would double-init).
    // This also constructs + Initialize()s the 3 custom services (render / beat /
    // camera) before the level GameObjects, mirroring 25_GameWorld.
    mGameWorld.LoadLevel(mLevelFile);

    // Resolve the live services created by LoadLevel.
    mRenderService = mGameWorld.GetService<CriticalCore2DRenderService>();
    mCameraShake = mGameWorld.GetService<CameraShakeService>();
    mBeatService = mGameWorld.GetService<BeatService>();

    // CameraShakeService::Initialize(int,int) is a name-HIDING overload (the base
    // no-arg Service::Initialize() the auto-lifecycle calls is a no-op), so snap
    // the camera to the room centre / reset shake explicitly here.
    if (mCameraShake != nullptr)
    {
        mCameraShake->Initialize(256, 224);
    }

    mClock.Reset();
    mGameFlow.Initialize(&mGameWorld);
}

void GameState::ReloadLevel()
{
    // Drop the flow's static bridges (Core/Player/score sink) first so nothing
    // dangles, then tear the world down and rebuild from the level - a clean
    // restart back to the title/menu.
    mGameFlow.Terminate();
    mRenderService = nullptr;
    mCameraShake = nullptr;
    mBeatService = nullptr;

    mGameWorld.Terminate();
    LoadAndWire();
}

void GameState::Terminate()
{
    mGameFlow.Terminate();
    mGameWorld.Terminate();

    // Free the process-shared Core shader cache (compiled once, reused across
    // games). Must happen here at app shutdown - while the GPU device is still
    // alive - not in a process-static dtor that would run after teardown.
    CoreComponent::TerminateSharedResources();

    GuiComponent::SetActiveState(nullptr);
    mRenderService = nullptr;
    mCameraShake = nullptr;
    mBeatService = nullptr;
}

void GameState::Update(float deltaTime)
{
    // ESC -> clean restart back to the title/menu (dev path, like 25_GameWorld's
    // reload). One-shot per press.
    auto* input = Input::InputSystem::Get();

    // F1 toggles the debug overlay (hidden by default; see DebugUI()).
    if (input != nullptr && input->IsKeyPressed(Input::KeyCode::F1))
    {
        mShowDebugUI = !mShowDebugUI;
    }

    // ESC/Backspace returns to the title ONLY while a game is running (GameMaker
    // oGlobalController/Step_0.gml honors the back key only when global.inGame).
    // On the title ESC must do nothing; on the post-game leaderboard MenuComponent
    // owns the back key, so gating on IsInGame() stops this from stomping either.
    // Checked per render frame (App::SetQuitOnEscape(false) lets us see the edge).
    if (input != nullptr && mGameFlow.IsInGame() &&
        (input->IsKeyPressed(Input::KeyCode::ESCAPE) || input->IsKeyPressed(Input::KeyCode::BACKSPACE)))
    {
        // In-place abandon: reset run state + drop transient objects WITHOUT a
        // world rebuild. The old ReloadLevel() here recompiled 6 HLSL shaders +
        // reallocated the render targets + restreamed the music on the ESC frame
        // (the menu-return lag spike). ReturnToTitle keeps all persistent
        // services resident. mClock.Reset() drops the abandoned run's leftover
        // accumulator so the fresh title starts on a clean step boundary.
        mGameFlow.ReturnToTitle();
        mClock.Reset();
        return;
    }

    // Fixed 60Hz timestep. ALL gameplay runs inside the fixed step; render is
    // locked to ticks (no interpolation). steps == 0 on a fast frame just skips.
    const int steps = mClock.Advance(deltaTime);
    for (int i = 0; i < steps; ++i)
    {
        // 1. Flow polls the menu/Core/Player signals, spawns Core/Player/bubbles,
        //    and refreshes the static bridges + flags BEFORE the components run.
        mGameFlow.Update();

        // 2. World updates every component at the fixed step (MusicController beat,
        //    walls, Core, player, projectiles, particles, HUD).
        mGameWorld.Update(GameClock::kStep);

        // 3. Camera follows the live Core/Player (post-update positions), then we
        //    push its scene-space offset (follow + shake jitter) into the render
        //    service so the whole 256x224 scene tracks it before the upscale.
        if (mCameraShake != nullptr)
        {
            CoreComponent* core = mGameFlow.GetCore();
            PlayerComponent* player = mGameFlow.GetPlayer();
            const float px = (player != nullptr) ? player->CenterX() : kArenaCenterX;
            const float py = (player != nullptr) ? player->CenterY() : kArenaCenterY;
            const float cx = (core != nullptr) ? core->CenterX() : kArenaCenterX;
            const float cy = (core != nullptr) ? core->CenterY() : kArenaCenterY;

            mCameraShake->SetTargets(px, py, cx, cy);
            mCameraShake->Update();

            if (mRenderService != nullptr)
            {
                float offsetX = 0.0f;
                float offsetY = 0.0f;
                mCameraShake->GetCameraOffset(offsetX, offsetY);
                mRenderService->SetCameraOffset(offsetX, offsetY);
            }
        }
    }
}

void GameState::Render()
{
    // The CriticalCore2DRenderService::Render() (fired here) does BeginScene ->
    // depth-sorted Draw() of every renderable into the 256x224 RT -> EndScene ->
    // Present (letterboxed POINT upscale to the 768x672 backbuffer). No gameplay
    // is drawn to the backbuffer directly.
    mGameWorld.Render();
}

void GameState::DebugUI()
{
    // Hidden during normal play; F1 (handled in Update) reveals it for debugging.
    if (!mShowDebugUI)
    {
        return;
    }

    ImGui::Begin("Critical Core 2", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Round: %d", mGameFlow.GetRound());
    ImGui::Text("Score: %d", mGameFlow.GetScore());
    ImGui::Text("Lives: %d", mGameFlow.GetLives());
    ImGui::Text("InGame: %s", mGameFlow.IsInGame() ? "yes" : "no");

    if (mBeatService != nullptr)
    {
        ImGui::Text("Beat: %.2f (index %d)", mBeatService->AudioBeat(), mBeatService->BeatIndex());
        ImGui::Text("AudioTick: %s", mBeatService->AudioTick() ? "yes" : "no");
    }

    ImGui::Text("Frame: %llu  Time: %.2fs",
                static_cast<unsigned long long>(mClock.FrameCount()),
                mClock.Time());

    if (ImGui::Button("Reload (ESC)"))
    {
        ReloadLevel();
    }

    ImGui::End();
}
