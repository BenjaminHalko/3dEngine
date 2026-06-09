#pragma once

#include "Collision.h"
#include "Render2DComponent.h"

#include <Engine/Inc/Engine.h>

#include <array>

namespace Engine::CriticalCore
{

// ---------------------------------------------------------------------------
// EntityComponent - shared motion + collision base (port of GameMaker pEntity).
//
// Source: objects/pEntity/{Create_0.gml, Step_0.gml}. pEntity is the GameMaker
// PARENT object of oPlayer, oBubble, oFireball and oSpike; every moving body in
// Critical Core 2 inherits its per-step motion + arena/core collision from here.
// This component is the engine-side equivalent: Player/Bubble/Fireball/Spike
// (tasks 22/23/24) subclass EntityComponent and only add their own specifics.
//
// ABSTRACT BASE - NO SET_TYPE_ID:
//   pEntity itself is never instantiated; only its children are. Likewise this
//   class declares no SET_TYPE_ID and must NOT be registered in the factory on
//   its own. Each concrete subclass declares
//       SET_TYPE_ID(CustomComponentId::XxxComponent);
//   (PlayerComponent / BubbleComponent / FireballComponent / SpikeComponent),
//   so every live object carries its own unique component id. EntityComponent
//   only inherits Render2DComponent's GetTypeId() as an unused fallback.
//
// Fixed-step motion (pEntity/Step_0.gml):
//   * Velocity is in PIXELS PER FIXED STEP (1 step = 1/60 s, GameClock). There
//     is NO dt scaling - UpdateEntity() runs INSIDE the fixed step.
//   * The motion term `spdMult * coreScale` (Step_0.gml:63-64) gives spawned
//     bodies an extra outward push proportional to the core's current scale;
//     once `collide` latches, spdMult fades to 0 via ApproachFade (:59-62).
//   * `collide` is the "don't collide until outside the core" gate (:55-56):
//     bodies spawn INSIDE the core and may not hit walls/redirect until they
//     have first cleared the core circle.
//
// Collision (all via Collision.h - NO duplicated geometry/reflection math):
//   * Non-player: reflect off outer walls (ReflectOffWall + re-derive xSpd/ySpd
//     through LengthDir), and radial push-out off the core (CoreRedirectDir).
//   * Player: march into contact then stop (xSpd=ySpd=0); death is deferred to
//     the PlayerComponent override of OnWallTouched() (task 22) since it needs
//     player-only state (playerHasMoved, deathDelay).
//   * The base chooses reflection vs push-out+death purely from IsPlayer().
//
// Core scale coupling:
//   pEntity reads `oCore.scale` directly. To avoid a CIRCULAR include with the
//   CoreComponent (task 21), the scale is mirrored into a shared static here.
//   The CoreComponent pushes SetCoreScale(scale) ONCE per fixed step, BEFORE
//   entities update; task 34 wires that ordering in GameState.
// ---------------------------------------------------------------------------
class EntityComponent : public Render2DComponent
{
  public:
    void Initialize() override;
    void Terminate() override;
    void Deserialize(const rapidjson::Value& value) override;

    // One FIXED 60Hz step of shared motion + collision (pEntity/Step_0.gml).
    // Reads then writes the owning GameObject's TransformComponent (x,y). NO dt.
    void UpdateEntity();

    // Area-proportional radius: sqrt(mass / PI). Returns 0 for massless bodies
    // (e.g. spikes, whose collision uses a fixed sprite radius elsewhere).
    static float RadiusFromMass(float mass);
    float Radius() const;

    // Radius for WALL + CORE collision = the GameMaker sprite-mask radius (NOT the
    // dynamic mass radius). PlayerComponent overrides this to a fixed constant
    // (oPlayer's 16x16 mask); mass radius is then absorb/visual only.
    virtual float CollisionRadius() const
    {
        return Radius();
    }

    // The base picks NON-player reflection vs player push-out + death from this.
    // PlayerComponent (task 22) overrides to return true.
    virtual bool IsPlayer() const
    {
        return false;
    }

    // Shared oCore.scale. CoreComponent (task 21) sets this each fixed step,
    // before entities update; UpdateEntity()'s motion term and core test read it.
    static void SetCoreScale(float scale);
    static float GetCoreScale();

    // Live boss-wall cage (CoreComponent::BossWalls()). The flow publishes the
    // Core's segment pointer each fixed step (and clears it on teardown) so the
    // shared per-entity wall test reflects/kills off the boss cage just like the
    // outer walls. nullptr -> no boss walls (pre-spawn / post-teardown).
    static void SetBossWalls(const std::array<WallSegment, 8>* bossWalls);
    static const std::array<WallSegment, 8>* GetBossWalls();

  protected:
    // Fired AFTER push-out/reflection when a wall is touched, ONLY for IsPlayer()
    // entities. PlayerComponent (task 22) overrides to evaluate PlayerWallDeath()
    // and trigger GameOver(). Default: no-op (non-player bodies never call it).
    virtual void OnWallTouched(const WallHit& hit)
    {
        (void)hit;
    }

    // Fired when an IsPlayer() entity overlaps the Core. PlayerComponent overrides
    // it to GameOver (the user-requested "core kills you"); non-player bodies use
    // their own redirect path and never call it. Default: no-op.
    virtual void OnCoreTouched()
    {
    }

    // GameMaker pEntity fields (Create_0.gml). Subclasses tune these on spawn:
    //   pEntity default spdMult = 0; oBubble sets spdMult = 1.
    // The shared base defaults spdMult = 1 (the bubble case); subclasses that
    // want the no-push behaviour set spdMult = 0 in their own Initialize/spawn.
    float xSpd = 0.0f;    // velocity x (px / fixed step)
    float ySpd = 0.0f;    // velocity y (px / fixed step, y-DOWN)
    float spdMult = 1.0f; // core-scale push multiplier; fades to 0 once colliding
    float mass = 0.0f;    // area mass; radius = sqrt(mass / PI)
    bool collide = false; // gate: no wall/core collision until first OUTSIDE core

  private:
    // Writable position source. Render2DComponent caches a CONST transform for
    // drawing; EntityComponent needs a mutable one to integrate position.
    TransformComponent* mEntityTransform = nullptr;

    static float sCoreScale; // mirror of oCore.scale (set by CoreComponent)
};
} // namespace Engine::CriticalCore
