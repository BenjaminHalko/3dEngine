#pragma once

#include "Collision.h"
#include "EntityComponent.h"

#include <Engine/Inc/Engine.h>

#include <functional>

namespace Engine::CriticalCore
{
// ---------------------------------------------------------------------------
// PlayerComponent - port of GameMaker oPlayer (objects/oPlayer/*.gml).
//
// The player blob: an EntityComponent (pEntity child) that adds keyboard
// movement, a continuous mass-decay-to-death rule, a drifting trail particle
// emitter, and a fireball charge+fire weapon. Derives from EntityComponent so
// it inherits the shared per-fixed-step motion + arena/core collision; only the
// player-specific behaviour lives here.
//
// FIXED-STEP CONTRACT: Update(deltaTime) runs exactly ONCE per 60Hz fixed step
// (task 14 GameClock / task 34 GameState world.Update(kStep)). deltaTime is
// ignored - every timer/speed here is in per-fixed-step units, matching the
// source's 60fps logic. Update() internally calls EntityComponent::UpdateEntity()
// at the point GameMaker runs event_inherited() (oPlayer/Step_0.gml:6), i.e.
// BEFORE the deathDelay countdown and the movement input.
//
// FLOW COUPLING (task 27): the global round-state (gameOver, nextRound,
// roundIntro) and the Core centre live outside the player. The flow pushes them
// via the setters below each step, and reads back GameOverRequested() (the
// radius<1 / wall-death signal) plus PlayerHasMoved() (forwarded to the Core).
//
// BUBBLE ABSORB (task 23): collecting a weapon bubble grants the player extra
// mass and arms the fireball via AddMass()/RequestFireball()/AddPulse().
// ---------------------------------------------------------------------------
class PlayerComponent final : public EntityComponent
{
  public:
    SET_TYPE_ID(CustomComponentId::PlayerComponent);

    void Initialize() override;
    void Terminate() override;
    void Update(float deltaTime) override; // one call == one fixed step (GameClock::kStep)
    void DebugUI() override;

    void Deserialize(const rapidjson::Value& value) override;

    void Draw(Render2D& render2D) override;

    // EntityComponent picks player push-out + death (not reflection) from this.
    bool IsPlayer() const override
    {
        return true;
    }

    // oPlayer's collision mask is the FIXED sPlayerMask (16x16 ellipse, origin
    // (8,8) -> 7px reach toward the top wall), independent of the player's grown
    // mass. Using this for wall/core collision (instead of the ~12.6 mass radius)
    // is what keeps the (128,0) spawn safe and matches the original game.
    float CollisionRadius() const override
    {
        return kPlayerMaskRadius;
    }

    // ----- Bubble-absorb grants (task 23 bubble collection) -----
    // Add absorbed mass (oBubble collect -> oPlayer.mass += ...).
    void AddMass(float amount)
    {
        mass += amount;
    }
    // Arm the fireball weapon (oBubble weapon collect -> oPlayer.shootFireball = true).
    void RequestFireball()
    {
        mShootFireball = true;
    }
    // Bump the purple collect pulse (oBubble collect flash).
    void AddPulse(float amount)
    {
        mPulse += amount;
    }
    float Mass() const
    {
        return mass;
    }
    void SetMass(float value)
    {
        mass = value;
    }

    // ----- playerHasMoved (player owns it; flow forwards to the Core) -----
    bool PlayerHasMoved() const
    {
        return mPlayerHasMoved;
    }
    void SetPlayerHasMoved(bool moved)
    {
        mPlayerHasMoved = moved;
    }

    // ----- GameOver signal (read + cleared by the flow, task 27) -----
    // Raised when radius<1 (mass decay) or PlayerWallDeath() fires. The flow
    // polls/clears it OR uses the callback, then runs GameOver() (explode the
    // player, set global.gameOver). The player never self-destructs.
    bool GameOverRequested() const
    {
        return mGameOverRequested;
    }
    void ClearGameOverRequest()
    {
        mGameOverRequested = false;
    }
    void SetOnGameOver(std::function<void()> callback)
    {
        mOnGameOver = std::move(callback);
    }

    // ----- Flow setters (task 27 pushes these each fixed step) -----
    void SetGameOver(bool gameOver)
    {
        mGameOver = gameOver;
    }
    void SetNextRound(bool nextRound)
    {
        mNextRound = nextRound;
    }
    void SetRoundIntro(bool roundIntro)
    {
        mRoundIntro = roundIntro;
    }
    // Core centre, for fireball aim reference (oFireball self-aims toward it).
    void SetCoreCenter(float x, float y)
    {
        mCoreCenterX = x;
        mCoreCenterY = y;
    }

    // Current world position (flow reads this to push to the Core each step).
    float CenterX() const;
    float CenterY() const;

  protected:
    // Fired by EntityComponent AFTER push-out when the player touches a wall.
    // Evaluates PlayerWallDeath() (needs playerHasMoved + deathDelay) and raises
    // the GameOver signal (oPlayer is the player-context for pEntity:44-48). For
    // a boss-wall touch this kicks in once the spawn deathDelay (30 steps) runs
    // out - the player must clear the cage before it grows around them.
    void OnWallTouched(const WallHit& hit) override;

  private:
    void TriggerGameOver();
    void SpawnTrail(float px, float py, float radius);
    void SpawnFireball(float px, float py);

    // Fixed wall/core collision reach matching sPlayerMask (16x16 ellipse, origin
    // (8,8)): 7px down to the top wall leaves the same 1px spawn gap GameMaker has.
    static constexpr float kPlayerMaskRadius = 7.0f;

    // oPlayer/Create_0.gml state.
    float mPulse = 0.0f;        // purple collect pulse, decays to 0
    float mDeathDelay = 30.0f;  // grace frames before flipped-wall death (pEntity)
    float mOuterSize = 0.0f;    // smoothed outer aura ring radius
    bool mShootFireball = false;
    float mFireballChargeUp = 0.0f; // 0..1 charge; fires at 1
    int mState = 0;            // BUBBLE_STATE.NORMAL (0); kept for fidelity

    bool mPlayerHasMoved = false;

    // Smoothed tint: merge_color(c_white, #9400DD, pulse). Drawn tint for the blob.
    Engine::Graphics::Color mImageBlend = Engine::Graphics::Colors::White;

    // ----- Flow state (pushed by task 27) -----
    bool mGameOver = false;
    bool mNextRound = false;
    bool mRoundIntro = false;
    float mCoreCenterX = Arena::kCenterX;
    float mCoreCenterY = Arena::kCenterY;

    bool mGameOverRequested = false;
    std::function<void()> mOnGameOver;
};
} // namespace Engine::CriticalCore
