#pragma once

#include "CustomTypeIds.h"
#include "EntityComponent.h"

#include <Engine/Inc/Engine.h>

namespace Engine::CriticalCore
{
// ---------------------------------------------------------------------------
// SpikeComponent - the Core's spinning hazard projectile (port of objects/oSpike).
//
// A small (8x8 sSpike) massless body the Core fires ON THE BEAT. It inherits the
// shared pEntity motion (reflect off walls, push out of the Core) via its base
// EntityComponent, then spins (image_angle -= 10 / step) and resolves contact:
//   * It NEVER damages while overlapping the Core (the spawn guard) so a freshly
//     launched spike can't immediately hurt anything still inside the Core.
//   * On the first overlap with a BUBBLE it bursts that bubble and self-destructs.
//   * On the first overlap with the PLAYER it deals a point penalty (snPointLoss
//     SFX, score -500, player.mass -200, a negative score popup) and self-destructs.
//
// Derives EntityComponent (oSpike's Step calls event_inherited() => the pEntity
// motion runs first), and is registered/registered-against the parallel Bubble
// (task 23) and Player (task 22) ONLY through CombatRegistry (header-only) so this
// file has no compile-time dependency on those not-yet-authored components.
//
// SPAWN CONTRACT (spawned by the Core, via CoreComponent::SpawnProjectile ->
// spike.json -> ConsumeLaunch for the initial velocity):
//   GameObject* go = world.CreateGameObject("Spike",
//       "Assets/Templates/Objects/CriticalCore/spike.json");
//   go->GetComponent<TransformComponent>()->position = { spawnX, spawnY, 0 };
//   gPendingLaunches[go] = launchParams;  // (inside the Core)
//   go->Initialize();                     // drains its velocity from ConsumeLaunch
//
// Source mapping (objects/oSpike/Step_0.gml):
//   * :6      event_inherited()        -> EntityComponent::UpdateEntity() (pEntity motion).
//   * :8      image_angle -= 10.
//   * :10     guard !place_meeting(oCore) (don't damage while overlapping the Core).
//   * :11-14  instance_place(oBubble): if it's a plain bubble -> BurstBubble.
//   * :15-22  ...else if it's the player -> snPointLoss, score popup -500, score -500, mass -200.
//   * :24     instance_destroy() on ANY overlap (bubble OR player).
// ---------------------------------------------------------------------------
class SpikeComponent final : public EntityComponent
{
  public:
    SET_TYPE_ID(CustomComponentId::SpikeComponent);

    void Initialize() override;
    void Update(float deltaTime) override; // one call == one fixed step (GameClock::kStep)
    void Draw(Render2D& render2D) override;

  private:
    Graphics::TextureId mSpikeTexture = 0;
    bool mLoaded = false;
    float mImageAngle = 0.0f; // image_angle (deg); decremented 10 per step
    bool mDestroyed = false;
};
} // namespace Engine::CriticalCore
