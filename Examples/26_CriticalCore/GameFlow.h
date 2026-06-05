#pragma once

#include "GuiComponent.h"  // GuiState (value member)
#include "Leaderboard.h"   // Leaderboard (value member)

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine
{
class GameWorld;
} // namespace Engine

namespace Engine::CriticalCore
{
class CoreComponent;
class PlayerComponent;
class CameraShakeService;

// ---------------------------------------------------------------------------
// GameFlow - the global game-flow state machine (port of scripts/GameStart and
// scripts/GameOver, with oGlobalController's in-game gate).
//
// It owns the global game state (score / round / lives / inGame / gameOver /
// nextRound / roundIntro / PB), the round-lives-score loop, and the frame-timed
// transitions between rounds and deaths. It also WIRES the static bridges the
// entity components exposed each fixed step:
//   * CombatRegistry        : SetActiveCore / SetActivePlayer / ConsumeScoreDelta
//   * BubbleComponent       : SetPlayer / SetBossWalls / SetScoreSink (live Core walls)
//   * CoreComponent         : flow setters (round / playerHasMoved / gameOver /
//                             nextRound / roundIntro / player position) + the
//                             NextRound signal (NextRoundRequested / Clear)
//   * PlayerComponent       : flow setters + the GameOver signal (GameOverRequested)
//   * GuiState              : the HUD snapshot the GuiComponent reads (GetGuiState)
//
// FIXED-STEP CONTRACT: Update() runs exactly ONCE per 60Hz GameClock step (task
// 14 / task 34 GameState world.Update(kStep) loop). It is NOT driven by a real /
// wall-clock timer. All timed transitions are scheduled in FIXED-STEP FRAME COUNTS
// (port of GameMaker call_later(..., time_source_units_frames)) and ticked from
// Update(), so they advance with the GameClock and never with real time.
//
// OWNERSHIP / WIRING (task 34 GameState):
//   GameFlow flow;
//   flow.Initialize(&mGameWorld);            // before the first Update()
//   gui->SetState(flow.GetGuiState());       // hand the HUD snapshot to oGUI
//   ... per fixed step:
//   cameraSvc->SetTargets(player.x, player.y, core.x, core.y); // uses GetPlayer()/GetCore()
//   flow.Update();
//   ... on teardown:
//   flow.Terminate();
//
// GameFlow SPAWNS the Core (GameStart) and the Player (Respawn) itself via
// GameWorld::CreateGameObject + the JSON templates, caches their components, and
// exposes them through GetCore() / GetPlayer() so the camera (task 34) can follow
// them. It NEVER renders (no draws live here).
// ---------------------------------------------------------------------------
class GameFlow
{
  public:
    // Cache the world, load the local leaderboard (PB + username), seed the HUD
    // state, and wire the BubbleComponent score sink. Call once before Update().
    void Initialize(Engine::GameWorld* world);

    // Clear the static bridges so no component holds a dangling Core/Player after
    // the world tears down. Call from GameState::Terminate.
    void Terminate();

    // One fixed GameClock step: tick the scheduler, poll the menu/Core/Player
    // signals, mediate the flow state, and push it into the components + GuiState.
    void Update();

    // ----- Flow functions (port of GameStart.gml + GameOver.gml) -----
    void GameStart();                               // spawn Core + 5-bubble ring (one WEAPON), reset state
    void Respawn();                                 // spawn the player + RoundStart
    void RoundStart();                              // begin a round (intro, Core BeginRound, delayed intro-end)
    void NextRound();                               // +score, round++, burst bubbles, schedule the next RoundStart
    void GameOver(bool instant = false);            // burst bubbles, snHit, ScreenShake, delayed PlayerExplode/RestartRound
    void RestartRound();                            // delayed: lose a life -> Respawn or GameEnd
    void GameEnd();                                 // destroy + Leaderboard Post + return to the menu/title
    void PlayerExplode(bool small = false);         // explode + destroy the player (snExplode, particles)
    void FireballCollect(float x, float y, float radius); // orange burst (parallel to FireballComponent's own)

    // ----- Hand-off to task 34 -----
    GuiState* GetGuiState()
    {
        return &mGuiState;
    }
    int GetScore() const
    {
        return mScore;
    }
    int GetRound() const
    {
        return mRound;
    }
    int GetLives() const
    {
        return mLives;
    }
    bool IsInGame() const
    {
        return mInGame;
    }
    CoreComponent* GetCore() const
    {
        return mCore;
    }
    PlayerComponent* GetPlayer() const
    {
        return mPlayer;
    }

  private:
    // Frame-timed scheduler (port of GameMaker call_later in frame units). Each
    // callback fires N fixed steps after it was registered and is self-guarded by
    // the `if (global.inGame)` check the source uses.
    struct ScheduledCall
    {
        uint64_t dueFrame = 0;
        std::function<void()> callback;
    };
    void ScheduleAfter(int frames, std::function<void()> callback);
    void TickScheduler();

    // Spawn / destroy helpers.
    CoreComponent* SpawnCore();
    PlayerComponent* SpawnPlayer();
    void SpawnBubbleRing();
    void BurstAllBubbles();
    void DestroyCore();
    void DestroyPlayer();

    // Per-step state push.
    void PushComponentState(); // flow setters -> Core/Player
    void PushGuiState();       // flow state -> GuiState (HUD snapshot)

    // Camera-shake passthrough (resolved lazily from the world).
    void ScreenShake(float magnitude, int frames);

    Engine::GameWorld* mWorld = nullptr;
    CoreComponent* mCore = nullptr;
    PlayerComponent* mPlayer = nullptr;
    CameraShakeService* mCameraShake = nullptr;

    // Global game state (mirror of GameMaker global.*).
    int mScore = 0;
    int mRound = 1;
    int mLives = 3;
    bool mInGame = false;
    bool mGameOver = false;
    bool mNextRound = false;
    bool mRoundIntro = false;

    GuiState mGuiState;       // the HUD snapshot (handed to GuiComponent by task 34)
    Leaderboard mLeaderboard; // local JSON save (PB + username), task 30

    std::vector<ScheduledCall> mScheduled;
    uint64_t mFrame = 0; // fixed-step counter (== GameClock frame units)
};
} // namespace Engine::CriticalCore
