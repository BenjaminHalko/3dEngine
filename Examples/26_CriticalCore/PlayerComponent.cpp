#include "PlayerComponent.h"

#include "Collision.h"
#include "GmHelpers.h"
#include "Render2D.h"
#include "TrailComponent.h"

#include <algorithm>
#include <cmath>

using namespace Engine;

namespace Engine::CriticalCore
{
namespace
{
// merge_color target for the purple collect pulse (#9400DD).
const Graphics::Color kPulseColor(148.0f / 255.0f, 0.0f / 255.0f, 221.0f / 255.0f, 1.0f);

// Reads a held key through either of its two bindings (arrows OR WASD), GML's
// Input() keyboard branch (scripts/Input/Input.gml:20-23; gamepad branch dropped).
int HeldKey(Input::KeyCode primary, Input::KeyCode alt)
{
    const Input::InputSystem* input = Input::InputSystem::Get();
    if (input == nullptr)
    {
        return 0;
    }
    return (input->IsKeyDown(primary) || input->IsKeyDown(alt)) ? 1 : 0;
}
} // namespace

void PlayerComponent::Initialize()
{
    // Base caches the (const + mutable) transform and registers with the render
    // service. EntityComponent owns the per-step motion + collision.
    EntityComponent::Initialize();

    // oPlayer/Create_0.gml + event_inherited (pEntity default spdMult = 0; the
    // player takes NO core-scale launch push, unlike bubbles which set 1).
    spdMult = 0.0f;
    if (mass <= 0.0f)
    {
        mass = 500.0f; // oPlayer/Create_0.gml:5 (unless the template overrode Mass).
    }

    mPulse = 0.0f;
    mDeathDelay = 30.0f;          // oPlayer/Create_0.gml:8
    mOuterSize = Radius();        // oPlayer/Create_0.gml:10 (outerSize = radius)
    mShootFireball = false;       // :13
    mFireballChargeUp = 0.0f;     // :14
    mState = 0;                   // BUBBLE_STATE.NORMAL (:16)
    mImageBlend = Engine::Graphics::Colors::White;
}

void PlayerComponent::Terminate()
{
    mOnGameOver = nullptr;
    EntityComponent::Terminate();
}

float PlayerComponent::CenterX() const
{
    return GetWorldX();
}

float PlayerComponent::CenterY() const
{
    return GetWorldY();
}

void PlayerComponent::Update(float deltaTime)
{
    (void)deltaTime; // fixed-step logic: one Update == one GameMaker step.

    // oPlayer/Step_0.gml:3 — if (global.gameOver) exit.
    if (mGameOver)
    {
        return;
    }

    // oPlayer/Step_0.gml:6 — event_inherited(): pEntity motion + arena/core
    // collision. May fire OnWallTouched() (player death hook) with the CURRENT
    // deathDelay, BEFORE it is decremented below (matches the GML event order).
    UpdateEntity();

    // oPlayer/Step_0.gml:8 — deathDelay = max(0, deathDelay - 1).
    mDeathDelay = std::max(0.0f, mDeathDelay - 1.0f);

    // oPlayer/Step_0.gml:11-13 — Input() then ApproachFade the velocity toward
    // (keyRight - keyLeft) * 2 / (keyDown - keyUp) * 2. Keyboard only (no gamepad).
    const int keyLeft = HeldKey(Input::KeyCode::LEFT, Input::KeyCode::A);
    const int keyRight = HeldKey(Input::KeyCode::RIGHT, Input::KeyCode::D);
    const int keyUp = HeldKey(Input::KeyCode::UP, Input::KeyCode::W);
    const int keyDown = HeldKey(Input::KeyCode::DOWN, Input::KeyCode::S);

    xSpd = ApproachFade(xSpd, static_cast<float>(keyRight - keyLeft) * 2.0f, 1.0f, 0.7f);
    ySpd = ApproachFade(ySpd, static_cast<float>(keyDown - keyUp) * 2.0f, 1.0f, 0.7f);

    // oPlayer/Step_0.gml:15-19 — first input latches playerHasMoved (the Core's
    // shoot gate). The GUI tutorial-clear coupling is mediated by the flow (it
    // reads PlayerHasMoved() and forwards to the Core + GUI), kept out of here.
    if (keyLeft != 0 || keyRight != 0 || keyUp != 0 || keyDown != 0)
    {
        mPlayerHasMoved = true;
    }

    // oPlayer/Step_0.gml:21-28 — continuous mass decay -> death, ONLY while in a
    // live round (not roundIntro / nextRound) AND after the player has moved.
    if (!mNextRound && !mRoundIntro && mPlayerHasMoved)
    {
        const float dist = PointDistance(0.0f, 0.0f, xSpd, ySpd) / 3.0f;
        mass -= dist + mass / 500.0f;
        if (Radius() < 1.0f)
        {
            TriggerGameOver(); // GameOver(true)
        }
    }

    // oPlayer/Step_0.gml:30-32 — fade the purple pulse, recompute image_blend.
    mPulse = ApproachFade(mPulse, 0.0f, 0.05f, 0.7f);
    mImageBlend = MergeColor(Engine::Graphics::Colors::White, kPulseColor, mPulse);

    // oPlayer/Step_0.gml:34-41 — trail emitter (BROWSER == 0 on desktop, so the
    // spawn chance is irandom(2) == 0). The trail is the c_ltgrey OUTLINE ring,
    // placed within radius*0.55 of the player, sized radius/3.
    if (IRandom(2) == 0)
    {
        const float dir = RandomRange(0.0f, 360.0f);
        const float len = RandomRange(0.0f, Radius() * 0.55f);
        const float px = GetWorldX() + LengthDirX(len, dir);
        const float py = GetWorldY() + LengthDirY(len, dir);
        SpawnTrail(px, py, Radius() / 3.0f);
    }

    // oPlayer/Step_0.gml:43-54 — fireball charge / fire. Charging takes 5 steps
    // (Approach(...,1,0.2)); on full charge it costs 50% mass and spawns a
    // fireball at the player (the fireball aims itself toward the Core).
    if (mShootFireball)
    {
        mFireballChargeUp = Approach(mFireballChargeUp, 1.0f, 0.2f);
        if (mFireballChargeUp >= 1.0f)
        {
            mShootFireball = false;
            mass -= mass * 0.5f;
            SpawnFireball(GetWorldX(), GetWorldY());
        }
    }
    else
    {
        mFireballChargeUp = Approach(mFireballChargeUp, 0.0f, 0.1f);
    }

    // oPlayer/Step_0.gml:58-60 — outerSize tracks radius (snaps up, eases down).
    const float radius = Radius();
    if (radius > mOuterSize)
    {
        mOuterSize = radius;
    }
    mOuterSize = ApproachFade(mOuterSize, radius, 0.5f, 0.8f);
}

void PlayerComponent::Draw(Render2D& render2D)
{
    // True draw position (world + camera follow/shake offset).
    float x = 0.0f;
    float y = 0.0f;
    GetDrawPosition(x, y);

    const float radius = Radius();

    // oPlayer/Draw_0.gml:3-11 — the outer aura ring, drawn in a desaturated copy
    // of image_blend (saturation * 0.3 via GameMaker HSV).
    Graphics::Color outerColor = MakeColorHSV(
        ColorGetHue(mImageBlend), ColorGetSat(mImageBlend) * 0.3f, ColorGetValue(mImageBlend));
    outerColor.a = mImageBlend.a; // MakeColorHSV forces alpha = 1; restore.
    if (mOuterSize > 0.0f)
    {
        render2D.DrawCircleOutline(x, y, mOuterSize, outerColor);
    }

    // oPlayer/Draw_0.gml:13-40 — the blob. The shCore volumetric shader is
    // replaced by a flat filled disc tinted by image_blend (Render2D primitives
    // only, per task). The blob jitters by +/-3px * fireballChargeUp while
    // charging (OPERA == 0 on desktop so the -_offset term is 0).
    if (radius > 0.0f)
    {
        const float jx = x + RandomRange(-3.0f, 3.0f) * mFireballChargeUp;
        const float jy = y + RandomRange(-3.0f, 3.0f) * mFireballChargeUp;
        render2D.DrawCircleFilled(jx, jy, radius, mImageBlend, /*outline=*/false);

        // oPlayer/Draw_0.gml:42 — the inner ring at the true (un-jittered) pos.
        render2D.DrawCircleOutline(x, y, radius, mImageBlend);
    }
}

void PlayerComponent::OnWallTouched(const WallHit& hit)
{
    // pEntity/Step_0.gml:44-48 (player context): die on touching a non-flipped
    // (outer) wall once moved, OR any wall once deathDelay has elapsed.
    if (PlayerWallDeath(hit.hit, mPlayerHasMoved, hit.flipped, mDeathDelay))
    {
        TriggerGameOver();
    }
}

void PlayerComponent::OnCoreTouched()
{
    TriggerGameOver();
}

void PlayerComponent::TriggerGameOver()
{
    if (mGameOverRequested)
    {
        return; // one-shot: the flow runs GameOver() exactly once.
    }
    mGameOverRequested = true;
    if (mOnGameOver)
    {
        mOnGameOver();
    }
}

void PlayerComponent::SpawnTrail(float px, float py, float radius)
{
    // oPlayer/Step_0.gml:38-40 — instance_create_depth(.., oPlayerTrail). The
    // default trail is the c_ltgrey OUTLINE ring with a random fade speed and no
    // drift (TrailComponent defaults), sized radius/3.
    GameObject* go =
        GetOwner().GetWorld().CreateGameObject("trail", "Assets/Templates/Objects/CriticalCore/trail.json");
    if (go == nullptr)
    {
        return;
    }
    if (TransformComponent* transform = go->GetComponent<TransformComponent>())
    {
        transform->position.x = px;
        transform->position.y = py;
        transform->position.z = 0.0f;
    }
    if (TrailComponent* trail = go->GetComponent<TrailComponent>())
    {
        trail->SetRadius(radius);
        // Outline/fade/drift keep the trail.json defaults (movement trail).
    }
    go->Initialize();
}

void PlayerComponent::SpawnFireball(float px, float py)
{
    // oPlayer/Step_0.gml:49-50 — instance_create_depth(x, y, oFireball). The
    // fireball aims itself toward the Core in its OWN Initialize (oFireball
    // Create_0.gml:9 point_direction(x,y,oCore.x,oCore.y)) and sets its orange
    // image_blend, so the player only spawns it at its own position.
    GameObject* go = GetOwner().GetWorld().CreateGameObject(
        "fireball", "Assets/Templates/Objects/CriticalCore/fireball.json");
    if (go == nullptr)
    {
        return;
    }
    if (TransformComponent* transform = go->GetComponent<TransformComponent>())
    {
        transform->position.x = px;
        transform->position.y = py;
        transform->position.z = 0.0f;
    }
    go->Initialize();
}

void PlayerComponent::DebugUI()
{
    ImGui::Text("Player mass=%.1f radius=%.2f", mass, Radius());
    ImGui::Text("moved=%d deathDelay=%.0f charge=%.2f", mPlayerHasMoved ? 1 : 0, mDeathDelay,
                mFireballChargeUp);
    ImGui::Text("gameOverReq=%d nextRound=%d roundIntro=%d", mGameOverRequested ? 1 : 0,
                mNextRound ? 1 : 0, mRoundIntro ? 1 : 0);
}

void PlayerComponent::Deserialize(const rapidjson::Value& value)
{
    // EntityComponent reads "Depth" + "Mass" + "SpdMult". The player template
    // ships Mass 500; Initialize() backfills it if the template omitted Mass.
    EntityComponent::Deserialize(value);
}
} // namespace Engine::CriticalCore
