#pragma once

#include "CustomTypeIds.h"
#include "Render2DComponent.h"

#include <Engine/Inc/Engine.h>

namespace Engine::CriticalCore
{
// ---------------------------------------------------------------------------
// TrailComponent - port of oPlayerTrail (objects/oPlayerTrail/*.gml).
//
// A short-lived 2D particle: a shrinking, fading circle that optionally drifts
// along a fixed heading. Two visual modes mirror the source's image_blend test
// (oPlayerTrail/Draw_0.gml):
//   * Outline mode (default, GameMaker c_ltgrey): a 1px ring via DrawCircleOutline.
//     This is the player's movement trail (spawned every step the player moves).
//   * Filled mode (any other blend colour): a solid disc via DrawCircleFilled.
//     This is the burst particle (BurstBubble / PlayerExplode / FireballCollect).
//
// Per fixed step (oPlayerTrail/Step_0.gml + GameMaker built-in speed/direction
// motion): drift the transform by (speed, direction), shrink/fade percent by
// `spd`, and self-destruct the owning GameObject when percent <= 0.
//
// SPAWN CONTRACT (the burst FUNCTIONS live in flow/entities - tasks 22/23/24/27 -
// but they spawn THIS type). Create the object, configure it through the setters
// below, position its TransformComponent, then call GameObject::Initialize():
//
//   GameObject* go = world.CreateGameObject("trail",
//       "Assets/Templates/Objects/CriticalCore/trail.json");
//   auto* t = go->GetComponent<TrailComponent>();
//   auto* tr = go->GetComponent<TransformComponent>();
//   tr->position = { px, py, 0.0f };
//   t->SetRadius(r); t->SetFadeSpeed(spd); t->SetDrift(speed, dirDeg);
//   t->SetColor(col); t->SetOutline(false);
//   go->Initialize();
//
//   * BurstBubble  (task 22): repeat max(10, mass/50). pos = bubble + lengthdir(
//       random(radius*0.8), random(360)). radius = random_range(R/4, R/3),
//       colour c_gray, filled. spd left default (random 0.02..0.04), no drift.
//   * PlayerExplode(task 23): repeat _small?20:clamp(mass/4,80,150). R=max(12,radius);
//       radius=random_range(R/5,R/3), speed=random(2), direction=random(360),
//       colour choose(c_white,c_aqua), spd=random_range(0.01,0.02), filled.
//       (_small halves speed & radius.) Plus one plain trail at the player origin.
//   * FireballCollect(task 24/27): repeat 50. R=max(12,radius); radius=random_range(
//       R/5,R/3), speed=random(2), direction=random(360), colour choose(#EE8213,
//       #EEA612), spd=random_range(0.02,0.05), filled.
// ---------------------------------------------------------------------------

class TrailComponent final : public Render2DComponent
{
  public:
    SET_TYPE_ID(CustomComponentId::TrailComponent);

    void Initialize() override;
    void Update(float deltaTime) override;
    void Draw(Render2D& render2D) override;

    void Deserialize(const rapidjson::Value& value) override;

    // --- Spawn configuration (call after CreateGameObject, before Initialize) --
    void SetRadius(float radius)
    {
        mRadius = radius;
    }
    // percent decrement per fixed step (oPlayerTrail.spd). Disables the default
    // random 0.02..0.04 roll done in Initialize.
    void SetFadeSpeed(float fadeSpeed)
    {
        mFadeSpeed = fadeSpeed;
    }
    // GameMaker built-in speed/direction drift (pixels-per-step, degrees y-down).
    void SetDrift(float speed, float directionDeg)
    {
        mDriftSpeed = speed;
        mDirection = directionDeg;
    }
    void SetColor(const Graphics::Color& color)
    {
        mColor = color;
    }
    // true = 1px ring (player movement trail); false = filled disc (burst).
    void SetOutline(bool outline)
    {
        mOutline = outline;
    }

  private:
    // Mutable transform (base only caches a const one); used to apply drift.
    TransformComponent* mMovableTransform = nullptr;

    float mRadius = 4.0f;    // oPlayerTrail.radius
    float mPercent = 1.0f;   // oPlayerTrail.percent (shrink + fade scalar)
    float mFadeSpeed = -1.0f; // oPlayerTrail.spd; <0 sentinel => roll in Initialize
    float mDriftSpeed = 0.0f; // GameMaker built-in speed
    float mDirection = 0.0f;  // GameMaker built-in direction (degrees)
    bool mOutline = true;     // c_ltgrey branch (ring) by default
    bool mDestroyed = false;

    Graphics::Color mColor = Graphics::Color(0.75294f, 0.75294f, 0.75294f, 1.0f); // c_ltgrey
};
} // namespace Engine::CriticalCore
