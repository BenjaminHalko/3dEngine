#pragma once

#include "CustomTypeIds.h"
#include "Render2DComponent.h"

#include <Engine/Inc/Engine.h>

#include <array>

namespace Engine::CriticalCore
{
// ---------------------------------------------------------------------------
// SparkleComponent - port of oSparkle (objects/oSparkle/*.gml).
//
// A one-shot animated sprite particle. On create (oSparkle/Create_0.gml) it
// randomly picks one of two 5-frame sprite sets (sSparkle / sSparkle2), a random
// rotation, a random start frame (0 or 1) and a near-1.0 frame speed. Each fixed
// step it advances the animation; when the animation passes the last frame the
// GameMaker "Animation End" event (oSparkle/Other_7.gml) destroys it - here we
// self-destruct the owning GameObject.
//
// All parameters are randomised in Initialize, so there is NOTHING to configure
// at spawn time. SPAWN CONTRACT: create the object, set its TransformComponent
// position, then call GameObject::Initialize():
//
//   GameObject* go = world.CreateGameObject("sparkle",
//       "Assets/Templates/Objects/CriticalCore/sparkle.json");
//   go->GetComponent<TransformComponent>()->position = { px, py, 0.0f };
//   go->Initialize();
//
// Sprite manifest (task 2): sSparkle / sSparkle2 = 3x3, origin (0,0), 5 numbered
// frames (sSparkle_0.png .. sSparkle_4.png).
// ---------------------------------------------------------------------------

class SparkleComponent final : public Render2DComponent
{
  public:
    SET_TYPE_ID(CustomComponentId::SparkleComponent);

    void Initialize() override;
    void Update(float deltaTime) override;
    void Draw(Render2D& render2D) override;

  private:
    static constexpr int kFrameCount = 5; // sprite_get_number(sSparkle) == 5

    std::array<Graphics::TextureId, kFrameCount> mFrames{};
    bool mLoaded = false;

    float mFrame = 0.0f;       // image_index (fractional)
    float mFrameSpeed = 1.0f;  // image_speed (frames per step)
    float mAngle = 0.0f;       // image_angle (degrees)
    bool mDestroyed = false;
};
} // namespace Engine::CriticalCore
