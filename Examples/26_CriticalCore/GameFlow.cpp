#include "GameFlow.h"

#include "BubbleComponent.h"
#include "CameraShakeService.h"
#include "Collision.h" // Arena centre + WallSegment (boss-wall bridge)
#include "CombatRegistry.h"
#include "CoreComponent.h"
#include "GmHelpers.h" // LengthDirX/Y, RandomRange, IRandom
#include "MenuComponent.h"
#include "PlayerComponent.h"
#include "SpikeComponent.h"
#include "TrailComponent.h"

#include <Engine/Inc/Engine.h>

#include <cmath>

using namespace Engine;

namespace Engine::CriticalCore
{
namespace
{
// Object template paths (Assets/Templates/Objects/CriticalCore, relative to the
// build/bin working directory). The ring templates are NEW (this task) - the Core
// can only inject mass/weapon through its private launch registry, and the bubble
// exposes no mass/state setter, so the GameStart ring (mass 150, one WEAPON) is
// data-driven through these dedicated templates (mass/state read via Deserialize).
constexpr const char* kCoreTemplate = "Assets/Templates/Objects/CriticalCore/core.json";
constexpr const char* kPlayerTemplate = "Assets/Templates/Objects/CriticalCore/player.json";
constexpr const char* kBubbleRingTemplate = "Assets/Templates/Objects/CriticalCore/bubble_ring.json";
constexpr const char* kBubbleWeaponTemplate = "Assets/Templates/Objects/CriticalCore/bubble_weapon.json";
constexpr const char* kTrailTemplate = "Assets/Templates/Objects/CriticalCore/trail.json";

// Player render depth (player.json Depth -5); explosion particles spawn at the
// player depth and depth+1 (GameOver.gml:96 / :102).
constexpr float kPlayerDepth = -5.0f;

// GameMaker colour constants used by the explosion / fireball-collect bursts.
const Graphics::Color kLtGrey{0.75294f, 0.75294f, 0.75294f, 1.0f}; // c_ltgrey
const Graphics::Color kWhite{1.0f, 1.0f, 1.0f, 1.0f};             // c_white
const Graphics::Color kAqua{0.0f, 1.0f, 1.0f, 1.0f};             // c_aqua
const Graphics::Color kOrange1{238.0f / 255.0f, 130.0f / 255.0f, 19.0f / 255.0f, 1.0f}; // #EE8213
const Graphics::Color kOrange2{238.0f / 255.0f, 166.0f / 255.0f, 18.0f / 255.0f, 1.0f}; // #EEA612

// --- SFX (Load-once static-local handles, same pattern as FireballComponent) ---
Engine::Audio::SoundId HitSfx()
{
    static Engine::Audio::SoundId id = Engine::Audio::SoundEffectManager::Get()->Load("CriticalCore/snHit.wav");
    return id;
}
Engine::Audio::SoundId StartSfx()
{
    static Engine::Audio::SoundId id = Engine::Audio::SoundEffectManager::Get()->Load("CriticalCore/snStart.wav");
    return id;
}
Engine::Audio::SoundId ExplodeSfx()
{
    static Engine::Audio::SoundId id = Engine::Audio::SoundEffectManager::Get()->Load("CriticalCore/snExplode.wav");
    return id;
}
void PlaySfx(Engine::Audio::SoundId id)
{
    Engine::Audio::SoundEffectManager::Get()->Play(id);
}

Graphics::Color ChooseWhiteAqua()
{
    return (IRandom(1) == 0) ? kWhite : kAqua; // choose(c_white, c_aqua)
}
Graphics::Color ChooseOrange()
{
    return (IRandom(1) == 0) ? kOrange1 : kOrange2; // choose(#EE8213, #EEA612)
}

// Spawn one oPlayerTrail particle (trail.json) and configure it. fadeSpeed < 0
// lets the trail roll its own random fade (oPlayerTrail default). Mirrors the
// shared task-26 spawn recipe (CreateGameObject -> set transform -> setters ->
// Initialize). CreateGameObject runs Deserialize but NOT Initialize.
void SpawnTrail(GameWorld& world,
                float x,
                float y,
                float radius,
                float driftSpeed,
                float driftDir,
                const Graphics::Color& color,
                float fadeSpeed,
                bool outline,
                float depth)
{
    GameObject* go = world.CreateGameObject("Trail", kTrailTemplate);
    if (go == nullptr)
    {
        return;
    }
    if (TransformComponent* transform = go->GetComponent<TransformComponent>())
    {
        transform->position.x = x;
        transform->position.y = y;
        transform->position.z = 0.0f;
    }
    if (TrailComponent* trail = go->GetComponent<TrailComponent>())
    {
        trail->SetRadius(radius);
        trail->SetFadeSpeed(fadeSpeed);
        trail->SetDrift(driftSpeed, driftDir);
        trail->SetColor(color);
        trail->SetOutline(outline);
        trail->SetDepth(depth);
    }
    go->Initialize();
}
} // namespace

void GameFlow::Initialize(Engine::GameWorld* world)
{
    mWorld = world;
    mCameraShake = (mWorld != nullptr) ? mWorld->GetService<CameraShakeService>() : nullptr;

    // Reset all flow state to a fresh TITLE (inGame=false). The flow instance
    // outlives a level reload (it is a GameState member), so an ESC->ReloadLevel
    // must clear the previous run's score/round/lives/flags here or the rebuilt
    // title would inherit a stale in-game state and never become interactive.
    mScore = 0;
    mRound = 1;
    mLives = 3;
    mInGame = false;
    mGameOver = false;
    mNextRound = false;
    mRoundIntro = false;
    mFrame = 0;
    mScheduled.clear();
    mCore = nullptr;
    mPlayer = nullptr;

    // Local save: PB + username (the menu writes the same file; we Load to read it).
    mLeaderboard.Load();

    // Route oBubble's `global.score += absorbAmount` (Step_0.gml:21) into our score.
    // The spike's point-loss goes through the CombatRegistry score-delta channel,
    // drained in Update(); both paths add to the same number.
    BubbleComponent::SetScoreSink([this](int delta) { mScore += delta; });

    // Seed the HUD snapshot.
    mGuiState = GuiState{};
    mGuiState.lives = mLives;
    mGuiState.round = mRound;
    mGuiState.pb = mLeaderboard.GetPB();
}

void GameFlow::Terminate()
{
    // Drop every static bridge so no component dereferences a stale Core/Player.
    SetActiveCore(nullptr);
    SetActivePlayer(nullptr);
    BubbleComponent::SetPlayer(nullptr);
    BubbleComponent::SetBossWalls(nullptr);
    BubbleComponent::SetScoreSink(nullptr);

    mScheduled.clear();
    mCore = nullptr;
    mPlayer = nullptr;
    mCameraShake = nullptr;
    mWorld = nullptr;
}

void GameFlow::Update()
{
    ++mFrame;

    // 1) Frame-timed transitions (port of call_later, time_source_units_frames).
    TickScheduler();

    // 2) Menu START signal -> GameStart (oMenu raises it; the flow runs the start).
    if (MenuComponent::ConsumeStartRequest())
    {
        GameStart();
    }

    // 3) Core HP-depleted -> NextRound (CoreFunctions DamageCore raises the latch).
    if (mCore != nullptr && mCore->NextRoundRequested())
    {
        mCore->ClearNextRoundRequest();
        NextRound();
    }

    // 4) Player death -> GameOver chain (radius<1 / wall-death raises the signal).
    if (mPlayer != nullptr && mPlayer->GameOverRequested())
    {
        mPlayer->ClearGameOverRequest();
        GameOver(false);
    }

    // 5) playerHasMoved mediation: the player owns it, the Core consumes it
    //    (oPlayer/Step_0.gml:16 `oCore.playerHasMoved = true`, flow-mediated).
    if (mPlayer != nullptr && mCore != nullptr)
    {
        mCore->SetPlayerHasMoved(mPlayer->PlayerHasMoved());
    }

    // 6) Push the flow state into the Core/Player components.
    PushComponentState();

    // 7) Combat registry: publish the live Core/Player + the bubble bridges, then
    //    drain the score-delta channel (spike point-loss etc.).
    SetActiveCore(mCore);
    SetActivePlayer(mPlayer);
    BubbleComponent::SetPlayer(mPlayer);
    BubbleComponent::SetBossWalls((mCore != nullptr) ? &mCore->BossWalls() : nullptr);
    mScore += ConsumeScoreDelta();

    // 8) Publish the HUD snapshot for oGUI.
    PushGuiState();
}

// ---------------------------------------------------------------------------
// GameStart.gml
// ---------------------------------------------------------------------------
void GameFlow::GameStart()
{
    // Fresh game: drop any pending transitions from a previous run.
    mScheduled.clear();

    // Spawn the Core at the arena centre (GameStart.gml:2).
    mCore = SpawnCore();

    // Reset global state (GameStart.gml:3-8).
    mLives = 3;
    mScore = 0;
    mRound = 1;
    mInGame = true;
    mNextRound = false;
    mGameOver = false;
    mRoundIntro = false;
    mGuiState.newPB = false;          // oGUI.newPB = false (GameStart.gml:8)
    mGuiState.displayExtraLives = true;

    // snStart: the menu raises the start signal, the flow plays the SFX (task 29).
    PlaySfx(StartSfx());

    // Respawn() spawns the player + runs RoundStart (GameStart.gml:9).
    Respawn();

    // Initial 5-bubble ring around the player, one WEAPON (GameStart.gml:11-20).
    SpawnBubbleRing();
}

void GameFlow::Respawn()
{
    // instance_destroy(pEntity) (GameStart.gml:68): the old player was already
    // exploded by the death chain; clear any straggler + the player bridges.
    DestroyPlayer();

    // Spawn the new player at (room_width/2, 0) = (128, 0) (GameStart.gml:69).
    mPlayer = SpawnPlayer();

    RoundStart();      // GameStart.gml:70
    mGameOver = false; // GameStart.gml:71
}

void GameFlow::RoundStart()
{
    // GameStart.gml:74-93.
    mNextRound = false;
    mRoundIntro = true;

    // with(oCore): playerHasMoved=false, shootDir=0, flipShootDir toggle,
    // targetScale=getCoreStart(round), hp=1, hpHit=0, reset dash - all in BeginRound.
    if (mCore != nullptr)
    {
        mCore->BeginRound(mRound);
    }

    // call_later(global.gameOver ? 60 : 90): end the intro; arm the GUI tutorial.
    // (Delay sampled at schedule time; the gameOver guard re-checked at fire time -
    // Respawn clears gameOver AFTER RoundStart, so the callback sees it false.)
    const int delay = mGameOver ? 60 : 90;
    ScheduleAfter(delay, [this]() {
        if (mInGame)
        {
            mRoundIntro = false;
            // (oGUI.alarm[1] = 180 move-tutorial arm is GuiComponent-internal; the
            //  flow holds no GuiComponent pointer - documented deviation.)
        }
    });
}

// ---------------------------------------------------------------------------
// GameOver.gml
// ---------------------------------------------------------------------------
void GameFlow::NextRound()
{
    // GameOver.gml:39-77.
    if (mGameOver)
    {
        return;
    }

    mNextRound = true;

    if (mLives < 3)
    {
        ++mLives;                          // GameOver.gml:42-44
        mGuiState.displayExtraLives = true;
    }
    else
    {
        mGuiState.displayExtraLives = false;
        mScore += 1500;                    // GameOver.gml:46-47
    }
    mScore += 10000;                       // GameOver.gml:49
    ++mRound;                              // GameOver.gml:50

    // with(oCore): new round + nextRound mode. The immediate targetScale/dash reset
    // (GameOver.gml:51-60) has no component setter; the Core eases to centre while
    // nextRound is set and BeginRound resets targetScale at the delayed RoundStart.
    if (mCore != nullptr)
    {
        mCore->SetRound(mRound);
        mCore->SetNextRound(true);
    }

    PlaySfx(StartSfx());                   // snStart (GameOver.gml:61); pitch 1.2 unsupported

    BurstAllBubbles();                     // GameOver.gml:62-65

    if (mPlayer != nullptr)
    {
        mPlayer->SetMass(500.0f);          // with(oPlayer) mass=500 (GameOver.gml:66-68)
    }

    SpikeComponent::DestroyAllSpikes();    // instance_destroy(oSpike) (GameOver.gml:69)

    ScheduleAfter(90, [this]() {           // GameOver.gml:71-75
        if (mInGame)
        {
            RoundStart();
        }
    });
}

void GameFlow::GameOver(bool instant)
{
    // GameOver.gml:1-37.
    if (mGameOver)
    {
        return; // GameOver.gml:4 guard
    }

    mGameOver = true;
    mNextRound = false;
    mRoundIntro = false;

    // (with(oCore) dash/hpHit reset (GameOver.gml:8-16) has no setter; the Core
    //  freezes while gameOver and RoundStart re-seeds it next round.)

    BurstAllBubbles();              // burst every non-player bubble (GameOver.gml:17-20)
    SpikeComponent::DestroyAllSpikes(); // instance_destroy(oSpike) (GameOver.gml:27,33)

    if (!instant)
    {
        PlaySfx(HitSfx());       // snHit (GameOver.gml:22)
        ScreenShake(4.0f, 20);   // GameOver.gml:23
        ScheduleAfter(30, [this]() {
            if (mInGame)
            {
                PlayerExplode(false);
                RestartRound();
            }
        });
    }
    else
    {
        PlayerExplode(true);     // GameOver.gml:32
        RestartRound();          // GameOver.gml:34
    }
}

void GameFlow::RestartRound()
{
    // GameOver.gml:135-145.
    ScheduleAfter(60, [this]() {
        if (mInGame)
        {
            --mLives;
            if (mLives <= 0)
            {
                GameEnd();
            }
            else
            {
                Respawn();
            }
        }
    });
}

void GameFlow::GameEnd()
{
    // GameStart.gml:23-28 (GameEnd) + GotoLeaderboard. The online board is cut, so
    // we submit to the local JSON leaderboard (task 30) and return to the title.
    mLeaderboard.Load(); // refresh from disk (the menu may have changed the username)
    const int oldPB = mLeaderboard.GetPB();
    const std::string username = mLeaderboard.GetUsername();
    mLeaderboard.Post(username, mScore); // == LeaderboardPost
    mLeaderboard.Save();
    mGuiState.newPB = (mScore > oldPB);

    // instance_destroy(oCore) + instance_destroy(pEntity) (GameStart.gml:24-25).
    DestroyCore();
    DestroyPlayer();
    BurstAllBubbles();

    // Drop the live-entity bridges.
    SetActiveCore(nullptr);
    SetActivePlayer(nullptr);
    BubbleComponent::SetPlayer(nullptr);
    BubbleComponent::SetBossWalls(nullptr);

    // Leave the in-game state (GotoLeaderboard / ReturnToMenu).
    mInGame = false;
    mGameOver = false;
    mNextRound = false;
    mRoundIntro = false;
    mGuiState.inGame = false;
    mGuiState.gameOver = false;

    // GotoLeaderboard (GameStart.gml:27): the dormant level menu reactivates onto
    // the post-game leaderboard, where ENTER starts a new game and ESC returns to
    // the title. The menu object already exists (level-placed), so we only signal
    // it - no second menu is created.
    MenuComponent::RequestGameOverLeaderboard();
}

void GameFlow::PlayerExplode(bool small)
{
    // GameOver.gml:93-116.
    ScreenShake(12.0f, 40);  // GameOver.gml:94
    PlaySfx(ExplodeSfx());   // snExplode (GameOver.gml:95)

    if (mPlayer == nullptr || mWorld == nullptr)
    {
        return;
    }

    const float px = mPlayer->CenterX();
    const float py = mPlayer->CenterY();
    const float playerRadius = mPlayer->Radius();
    const float mass = mPlayer->Mass();

    // One plain trail at the player origin (GameOver.gml:96).
    SpawnTrail(*mWorld, px, py, 4.0f, 0.0f, 0.0f, kLtGrey, /*fadeSpeed=*/-1.0f, /*outline=*/true, kPlayerDepth);

    // repeat(_small ? 20 : clamp(mass/4, 80, 150)) (GameOver.gml:98). BROWSER cap
    // ignored (desktop target).
    const float r = Math::Max(12.0f, playerRadius); // max(12, radius)
    const int count = small ? 20 : static_cast<int>(Math::Clamp(mass / 4.0f, 80.0f, 150.0f));
    for (int i = 0; i < count; ++i)
    {
        const float dir = RandomRange(0.0f, 360.0f);
        const float len = RandomRange(0.0f, r * 0.8f);
        const float ex = px + LengthDirX(len, dir);
        const float ey = py + LengthDirY(len, dir);

        float partRadius = RandomRange(r / 5.0f, r / 3.0f);
        float driftSpeed = RandomRange(0.0f, 2.0f); // speed = random(2)
        if (small)
        {
            driftSpeed /= 2.0f; // _small halves speed & radius (GameOver.gml:108-111)
            partRadius /= 2.0f;
        }
        const float fade = RandomRange(0.01f, 0.02f); // spd = random_range(0.01, 0.02)
        SpawnTrail(*mWorld, ex, ey, partRadius, driftSpeed, RandomRange(0.0f, 360.0f), ChooseWhiteAqua(), fade,
                   /*outline=*/false, kPlayerDepth + 1.0f);
    }

    // instance_destroy() (GameOver.gml:114): the player never self-destructs - the
    // flow removes it here.
    DestroyPlayer();
}

void GameFlow::FireballCollect(float x, float y, float radius)
{
    // GameOver.gml:118-133. NOTE: the LIVE fireball->wall path is owned by
    // FireballComponent (task 24), which carries its own copy of this burst; this
    // is the faithful flow-side port of the GameOver.gml function for completeness.
    if (mWorld == nullptr)
    {
        return;
    }

    ScreenShake(4.0f, 5); // GameOver.gml:119

    // One plain trail at the impact (GameOver.gml:120).
    SpawnTrail(*mWorld, x, y, 4.0f, 0.0f, 0.0f, kLtGrey, /*fadeSpeed=*/-1.0f, /*outline=*/true, kPlayerDepth);

    const float r = Math::Max(12.0f, radius); // max(12, radius)
    for (int i = 0; i < 50; ++i)              // repeat(50)
    {
        const float dir = RandomRange(0.0f, 360.0f);
        const float len = RandomRange(0.0f, r * 0.8f);
        const float px = x + LengthDirX(len, dir);
        const float py = y + LengthDirY(len, dir);
        SpawnTrail(*mWorld, px, py, RandomRange(r / 5.0f, r / 3.0f), RandomRange(0.0f, 2.0f),
                   RandomRange(0.0f, 360.0f), ChooseOrange(), RandomRange(0.02f, 0.05f), /*outline=*/false,
                   kPlayerDepth + 1.0f);
    }
}

// ---------------------------------------------------------------------------
// Spawn / destroy helpers
// ---------------------------------------------------------------------------
CoreComponent* GameFlow::SpawnCore()
{
    if (mWorld == nullptr)
    {
        return nullptr;
    }
    GameObject* go = mWorld->CreateGameObject("Core", kCoreTemplate);
    if (go == nullptr)
    {
        return nullptr;
    }
    if (TransformComponent* transform = go->GetComponent<TransformComponent>())
    {
        transform->position.x = Arena::kCenterX; // room_width/2 = 128
        transform->position.y = Arena::kCenterY; // room_height/2 = 112
        transform->position.z = 0.0f;
    }
    go->Initialize(); // CreateGameObject Deserializes but does NOT Initialize.
    CoreComponent* core = go->GetComponent<CoreComponent>();
    SetActiveCore(core);
    if (core != nullptr)
    {
        BubbleComponent::SetBossWalls(&core->BossWalls());
    }
    return core;
}

PlayerComponent* GameFlow::SpawnPlayer()
{
    if (mWorld == nullptr)
    {
        return nullptr;
    }
    GameObject* go = mWorld->CreateGameObject("Player", kPlayerTemplate);
    if (go == nullptr)
    {
        return nullptr;
    }
    if (TransformComponent* transform = go->GetComponent<TransformComponent>())
    {
        // GameStart.gml:69 spawns at (room_width/2, 0) = (128, 0), just above the
        // octagon's top wall. This is now SAFE because PlayerComponent collides with
        // its fixed sprite-mask radius (~7), not the dynamic mass radius - matching
        // the original game's 1px spawn gap (a prior fix wrongly moved the spawn onto
        // the Core at the arena centre).
        transform->position.x = Arena::kCenterX; // room_width/2 = 128
        transform->position.y = 0.0f;            // room_height topmost (GameStart.gml:69)
        transform->position.z = 0.0f;
    }
    go->Initialize();
    PlayerComponent* player = go->GetComponent<PlayerComponent>();
    SetActivePlayer(player);
    BubbleComponent::SetPlayer(player);
    return player;
}

void GameFlow::SpawnBubbleRing()
{
    // GameStart.gml:11-20. Five bubbles in a ring 25px from the player, one WEAPON.
    if (mWorld == nullptr || mPlayer == nullptr)
    {
        return;
    }
    const float px = mPlayer->CenterX();
    const float py = mPlayer->CenterY();
    constexpr float kRingDistance = 25.0f;
    for (int i = 0; i < 5; ++i)
    {
        const float dir = static_cast<float>(i) * 360.0f / 5.0f + 90.0f; // i*72 + 90
        const float bx = px + LengthDirX(kRingDistance, dir);
        const float by = py + LengthDirY(kRingDistance, dir);

        const bool weapon = (i == 0); // state = BUBBLE_STATE.WEAPON for i == 0
        const char* templatePath = weapon ? kBubbleWeaponTemplate : kBubbleRingTemplate;

        GameObject* go = mWorld->CreateGameObject("Bubble", templatePath);
        if (go == nullptr)
        {
            continue;
        }
        if (TransformComponent* transform = go->GetComponent<TransformComponent>())
        {
            transform->position.x = bx;
            transform->position.y = by;
            transform->position.z = 0.0f;
        }
        go->Initialize(); // mass 150 + state read from the template via Deserialize.
    }
}

void GameFlow::BurstAllBubbles()
{
    // Burst every live (non-player) bubble. BurstBubble defers destruction, so a
    // single pass over the registry is safe (notepad task 23).
    for (BubbleComponent* bubble : BubbleComponent::AllBubbles())
    {
        if (bubble != nullptr)
        {
            bubble->BurstBubble();
        }
    }
}

void GameFlow::DestroyCore()
{
    if (mCore != nullptr && mWorld != nullptr)
    {
        mWorld->DestroyGameObject(mCore->GetOwner().GetHandle());
    }
    mCore = nullptr;
    SetActiveCore(nullptr);
    BubbleComponent::SetBossWalls(nullptr);
}

void GameFlow::DestroyPlayer()
{
    if (mPlayer != nullptr && mWorld != nullptr)
    {
        mWorld->DestroyGameObject(mPlayer->GetOwner().GetHandle());
    }
    mPlayer = nullptr;
    SetActivePlayer(nullptr);
    BubbleComponent::SetPlayer(nullptr);
}

// ---------------------------------------------------------------------------
// Per-step state push
// ---------------------------------------------------------------------------
void GameFlow::PushComponentState()
{
    if (mCore != nullptr)
    {
        mCore->SetRound(mRound);
        mCore->SetGameOver(mGameOver);
        mCore->SetNextRound(mNextRound);
        mCore->SetRoundIntro(mRoundIntro);
        if (mPlayer != nullptr)
        {
            mCore->SetPlayerPosition(mPlayer->CenterX(), mPlayer->CenterY());
        }
        // (SetWeaponCount left at its default 0 -> weapons allowed; tracking the
        //  live weapon-bubble + fireball count needs registries not exposed.)
    }

    if (mPlayer != nullptr)
    {
        mPlayer->SetGameOver(mGameOver);
        mPlayer->SetNextRound(mNextRound);
        mPlayer->SetRoundIntro(mRoundIntro);
        if (mCore != nullptr)
        {
            mPlayer->SetCoreCenter(mCore->CenterX(), mCore->CenterY());
        }
    }
}

void GameFlow::PushGuiState()
{
    // Continuously-updated HUD fields. newPB / displayExtraLives are event-driven
    // (set in GameStart / NextRound / GameEnd) and intentionally not overwritten.
    mGuiState.score = mScore;
    mGuiState.lives = mLives;
    mGuiState.round = mRound;
    mGuiState.pb = mLeaderboard.GetPB();
    mGuiState.inGame = mInGame;
    mGuiState.gameOver = mGameOver;
    mGuiState.nextRound = mNextRound;
    mGuiState.roundIntro = mRoundIntro;
}

// ---------------------------------------------------------------------------
// Frame-timed scheduler (port of call_later, time_source_units_frames)
// ---------------------------------------------------------------------------
void GameFlow::ScheduleAfter(int frames, std::function<void()> callback)
{
    ScheduledCall call;
    call.dueFrame = mFrame + static_cast<uint64_t>((frames < 0) ? 0 : frames);
    call.callback = std::move(callback);
    mScheduled.push_back(std::move(call));
}

void GameFlow::TickScheduler()
{
    // Collect everything due this frame FIRST, then fire it - so a callback that
    // schedules a new call (RestartRound, NextRound->RoundStart) does not re-fire
    // in the same tick (its dueFrame is in the future).
    std::vector<std::function<void()>> due;
    for (auto it = mScheduled.begin(); it != mScheduled.end();)
    {
        if (it->dueFrame <= mFrame)
        {
            due.push_back(std::move(it->callback));
            it = mScheduled.erase(it);
        }
        else
        {
            ++it;
        }
    }
    for (auto& callback : due)
    {
        callback();
    }
}

// ---------------------------------------------------------------------------
// FX passthrough
// ---------------------------------------------------------------------------
void GameFlow::ScreenShake(float magnitude, int frames)
{
    if (mCameraShake == nullptr && mWorld != nullptr)
    {
        mCameraShake = mWorld->GetService<CameraShakeService>();
    }
    if (mCameraShake != nullptr)
    {
        mCameraShake->ScreenShake(magnitude, frames);
    }
}
} // namespace Engine::CriticalCore
