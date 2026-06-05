#pragma once

#include "CustomTypeIds.h"
#include "Render2DComponent.h"

#include <Engine/Inc/Engine.h>

namespace Engine::CriticalCore
{
class CoreComponent;
class CameraShakeService;

// ---------------------------------------------------------------------------
// FireballComponent - the player's weapon projectile (port of objects/oFireball).
//
// The player launches a Fireball that flies in a STRAIGHT line toward the Core
// at a fixed 6 px / fixed-step and explodes on the first wall it touches. If that
// wall is one of the Core's BOSS walls (the live octagon that grows with the
// Core) it deals DamageCore() damage; on any wall it runs FireballCollect (screen
// shake + 50 orange burst particles) and self-destructs. While in flight it lays
// down a wavy orange trail (a sine offset perpendicular to its heading).
//
// NOT an EntityComponent: oFireball OVERRIDES pEntity's Step entirely (its motion
// is the custom straight-line march, NOT the shared reflect/push-out), so this
// derives Render2DComponent directly and owns its own integration.
//
// SPAWN CONTRACT (oFireball is spawned by the Player, task 22 - NOT by the Core):
//   GameObject* go = world.CreateGameObject("Fireball",
//       "Assets/Templates/Objects/CriticalCore/fireball.json");
//   go->GetComponent<TransformComponent>()->position = { playerX, playerY, 0 };
//   go->GetComponent<FireballComponent>()->SetSourceMass(player.mass); // -> radius
//   go->Initialize();   // auto-aims at the active Core (CombatRegistry::GetActiveCore)
//
// Source mapping (objects/oFireball):
//   * Create_0.gml:5     snFireShoot SFX.
//   * Create_0.gml:6     radius = clamp(sqrt(max(0,oPlayer.mass)/pi), 10, 50).
//   * Create_0.gml:7     ScreenShake(4, 10).
//   * Create_0.gml:9     dir = point_direction(x, y, oCore.x, oCore.y).
//   * Step_0.gml:8-9     x += lengthdir_x(6, dir); y += lengthdir_y(6, dir).
//   * Step_0.gml:11-17   wall hit -> if boss wall DamageCore(); FireballCollect(); destroy.
//   * Step_0.gml:19-26   wavy trail: Wave(-1,1,1,0) * sprite_width/2 perpendicular offset.
//   * Draw_0.gml:3-4     drawCircle(x, y, radius) in image_blend (default white).
//   * GameOver.gml:118-133 FireballCollect: ScreenShake(4,5) + one plain trail + 50 orange.
// ---------------------------------------------------------------------------
class FireballComponent final : public Render2DComponent
{
  public:
    SET_TYPE_ID(CustomComponentId::FireballComponent);

    void Initialize() override;
    void Update(float deltaTime) override; // one call == one fixed step (GameClock::kStep)
    void Draw(Render2D& render2D) override;

    void Deserialize(const rapidjson::Value& value) override;

    // --- Spawn configuration (set by the Player before Initialize) ---
    // oPlayer.mass at launch; drives the drawn radius (Create_0.gml:6).
    void SetSourceMass(float mass)
    {
        mSourceMass = mass;
    }
    // Optional explicit Core (else the active Core from CombatRegistry is used).
    void SetCore(CoreComponent* core)
    {
        mCore = core;
    }

  private:
    // GameOver.gml FireballCollect: shake + one plain trail + 50 orange particles.
    void FireballCollect(float x, float y);

    TransformComponent* mTransform = nullptr; // mutable position source
    CoreComponent* mCore = nullptr;           // active Core (CombatRegistry default)
    CameraShakeService* mCameraShake = nullptr;

    float mDir = 0.0f;        // travel direction toward the Core (deg), set in Initialize
    float mRadius = 10.0f;    // drawn radius = clamp(sqrt(mass/pi), 10, 50)
    float mSourceMass = 0.0f; // oPlayer.mass at launch
    float mAge = 0.0f;        // seconds alive; drives the wavy-trail Wave phase
    bool mDestroyed = false;
};
} // namespace Engine::CriticalCore
