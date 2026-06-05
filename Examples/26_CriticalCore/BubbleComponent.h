#pragma once

#include "Collision.h"
#include "CustomTypeIds.h"
#include "EntityComponent.h"

#include <Engine/Inc/Engine.h>

#include <array>
#include <functional>
#include <vector>

namespace Engine::CriticalCore
{
class PlayerComponent; // task 22 - the bubble-like absorber (oPlayer is oBubble's child)

// ---------------------------------------------------------------------------
// BubbleComponent - port of GameMaker oBubble (objects/oBubble/*.gml).
//
// The arena fills with bubbles fired by the Core on the beat. A bubble drifts
// outward, merges with overlapping bubbles (the bigger eats the smaller), and is
// eventually absorbed by the player (who is, in the source, a CHILD object of
// oBubble - oPlayer/oPlayer.yy parentObjectId == oBubble - so the player IS a
// bubble-like absorber). Absorbing a bubble awards score and transfers its mass
// to the player; WEAPON bubbles also grant the player a fireball. A bubble caught
// between the growing Core (a flipped/inner wall) and the outer arena wall is
// SQUISHED and bursts into gray particles.
//
// Derives from EntityComponent (pEntity) for the shared per-step motion + arena/
// core collision; this class adds only the bubble specifics (merge/absorb, state
// tint, squish, radius easing).
//
// Source mapping:
//   * oBubble/Create_0.gml : state enum, mass/radius/absorber/absorbAmount, the
//                            WEAPON latch off oCore.timeSinceLastPurple.
//   * oBubble/Step_0.gml    : absorb() (:8-24), absorber-pull / pairwise merge /
//                            mass transfer with absorber+allowMerge gating
//                            (:26-110), squish-on-wall burst (:112-126), radius =
//                            sqrt(mass/pi) ease + destroy-if-mass<1 (:128-134).
//   * oBubble/Draw_0.gml    : filled circle tinted by state (aqua / lime / fuchsia).
//   * GameOver/GameOver.gml : BurstBubble (:79-91) - gray trail particles, count
//                            max(10, mass/50), then destroy.
//
// FIXED-STEP CONTRACT: Update(deltaTime) runs exactly ONCE per 60Hz fixed step
// (GameClock / GameState). deltaTime is ignored; every speed/timer is per-step.
//
// DECOUPLING (no GameObject iteration exists in the engine, and PlayerComponent /
// the game flow are separate tasks): like CoreComponent::ConsumeLaunch and
// ScoreComponent's live registry, BubbleComponent uses null-safe static bridges:
//   * a live-bubble registry for the bubble-vs-bubble overlap (instance_place);
//   * a PlayerAbsorbTarget the player publishes so bubbles can home into and feed
//     it without a compile-time PlayerComponent dependency;
//   * the Core's live boss walls (flipped/inner) for the squish test;
//   * a global-score sink the flow owns (global.score += absorbAmount).
// All bridges are optional: absent -> bubbles still merge with each other.
// ---------------------------------------------------------------------------
class BubbleComponent final : public EntityComponent
{
  public:
    SET_TYPE_ID(CustomComponentId::BubbleComponent);

    // oBubble/Create_0.gml:3-7. (Order matches the task spec; only NORMAL == 0
    // crosses a serialization boundary - the default bubble.json state.)
    enum class State
    {
        NORMAL,
        WEAPON,
        DOUBLE_POINTS
    };

    // --- Player bridge (PlayerComponent = task 22; flow/GameState wires it) ----
    // The player is a bubble-like absorber (oPlayer is an oBubble child): bubbles
    // home into it and, on contact, feed it the absorbed mass + a collect pulse,
    // and (WEAPON) arm its fireball - exactly the AddMass()/AddPulse()/
    // RequestFireball() grants PlayerComponent exposes. The engine has no
    // GameObject iteration, so the flow registers the live player here each fixed
    // step (or once). nullptr (no player yet / player dead) -> bubbles only merge
    // with each other; absorb is skipped. Position/radius come from the player's
    // CenterX()/CenterY()/Radius() (no duplicated state).
    static void SetPlayer(PlayerComponent* player); // nullptr clears
    static PlayerComponent* Player();

    // --- Boss-wall bridge (the Core owns the live flipped/inner walls) ---------
    // CircleVsOuterWalls only reports the 8 OUTER (non-flipped) walls, so the
    // squish needs the Core's runtime boss walls fed in. The flow pushes
    // &core->BossWalls() each fixed step. nullptr -> no inner wall -> no squish.
    static void SetBossWalls(const std::array<WallSegment, 8>* bossWalls);

    // --- Global score sink (the flow owns global.score) -----------------------
    // absorb() does `global.score += absorbAmount`; the flow wires this to the
    // HUD/GuiState score. nullptr -> the credited amount is dropped (popup still
    // shows). The popup VISUAL always routes through ScoreComponent (score.json).
    static void SetScoreSink(std::function<void(int)> sink);

    // Live bubble registry for the game-over mass-burst (GameOver bursts every
    // oBubble). BurstBubble defers destruction, so iterating this and bursting in
    // one pass is safe (Terminate erases only in the post-update destroy sweep).
    static const std::vector<BubbleComponent*>& AllBubbles();

    void Initialize() override;
    void Terminate() override;
    void Update(float deltaTime) override; // one call == one fixed step
    void Draw(Render2D& render2D) override;
    void Deserialize(const rapidjson::Value& value) override;

    // GameOver/GameOver.gml BurstBubble(:79-91): spawn gray trail particles
    // (count max(10, mass/50)) then destroy. PUBLIC so the spike (task 24, burst
    // on contact) and the flow (task 27, game over) can burst a bubble too.
    void BurstBubble();

    State GetState() const
    {
        return mState;
    }
    float GetMass() const
    {
        return mass;
    }

  private:
    void Absorb();                                   // absorb() (Step_0.gml:8-24)
    void EmitWeaponSparkle();                         // weapon FX (Step_0.gml:28-38)
    BubbleComponent* FindOverlappingBubble();         // instance_place(x,y,oBubble)
    bool PlayerOverlaps() const;                      // overlap test vs the player
    void TransferMass(BubbleComponent* other);        // pairwise merge (Step_0.gml:51-89)
    void SpawnScorePopup(int amount, bool isDouble, bool negative, float x, float y);
    void DestroySelf();

    State mState = State::NORMAL;
    bool mAllowMerge = false;   // allowMerge (Create_0.gml:12) - gates merging
    float mDrawRadius = 0.0f;   // eased radius (Step_0.gml:128); starts 0, grows
    bool mHasAbsorber = false;  // absorber != noone (the player is the only absorber)
    float mAbsorbAmount = 0.0f; // round(mass/2) latched at claim (Create_0.gml:16)
    bool mDestroyed = false;    // self-destruct guard (deferred destroy is one-shot)
};
} // namespace Engine::CriticalCore
