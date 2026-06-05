#include "BubbleComponent.h"

#include "CoreComponent.h"
#include "GmHelpers.h"
#include "PlayerComponent.h"
#include "Render2D.h"
#include "ScoreComponent.h"
#include "TrailComponent.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace Engine;

namespace Engine::CriticalCore
{
namespace
{
    // c_gray (GameOver.gml:86 BurstBubble particle tint) = 128/255.
    constexpr float kGray = 0.50196f;

    // Bubble homing speed once claimed by the player (Step_0.gml:98-99).
    constexpr float kHomeSpeed = 5.0f;

    // State tints (oBubble/Draw_0.gml): default aqua, double-points lime, weapon
    // fuchsia (all GameMaker built-in colour constants).
    const Graphics::Color kColorNormal{0.0f, 1.0f, 1.0f, 1.0f}; // c_aqua
    const Graphics::Color kColorDouble{0.0f, 1.0f, 0.0f, 1.0f}; // c_lime
    const Graphics::Color kColorWeapon{1.0f, 0.0f, 1.0f, 1.0f}; // c_fuchsia

    // --- Null-safe static bridges (see header) --------------------------------
    PlayerComponent* gPlayer = nullptr;
    const std::array<WallSegment, 8>* gBossWalls = nullptr;
    std::function<void(int)> gScoreSink;

    // Live oBubble instances (the instance_place(x,y,oBubble) source). Registered
    // in Initialize, removed in Terminate; never outlives the components it holds.
    std::vector<BubbleComponent*>& LiveBubbles()
    {
        static std::vector<BubbleComponent*> bubbles;
        return bubbles;
    }
} // namespace

void BubbleComponent::SetPlayer(PlayerComponent* player)
{
    gPlayer = player;
}

PlayerComponent* BubbleComponent::Player()
{
    return gPlayer;
}

void BubbleComponent::SetBossWalls(const std::array<WallSegment, 8>* bossWalls)
{
    gBossWalls = bossWalls;
}

void BubbleComponent::SetScoreSink(std::function<void(int)> sink)
{
    gScoreSink = std::move(sink);
}

const std::vector<BubbleComponent*>& BubbleComponent::AllBubbles()
{
    return LiveBubbles();
}

void BubbleComponent::Initialize()
{
    // Base: cache transforms (mutable + const) + register with the render service.
    EntityComponent::Initialize();

    // Consume the Core's launch params (velocity / mass / weapon flag). Decoupled
    // via the static registry; false when this bubble was level-placed, not
    // Core-spawned (then mass/state come from bubble.json via Deserialize).
    CoreComponent::LaunchParams launch;
    if (CoreComponent::ConsumeLaunch(&GetOwner(), launch))
    {
        xSpd = launch.xSpd;
        ySpd = launch.ySpd;
        mass = launch.mass;
        if (launch.weapon)
        {
            mState = State::WEAPON;
        }
    }

    // oBubble starts at radius 0 (Create_0.gml:14) and eases up to sqrt(mass/pi);
    // pEntity sets spdMult = 1 for oBubble (already the EntityComponent default).
    mDrawRadius = 0.0f;

    LiveBubbles().push_back(this);
}

void BubbleComponent::Terminate()
{
    std::vector<BubbleComponent*>& live = LiveBubbles();
    live.erase(std::remove(live.begin(), live.end(), this), live.end());
    EntityComponent::Terminate();
}

void BubbleComponent::Update(float deltaTime)
{
    (void)deltaTime; // fixed step - all motion/timers are per-step, never dt-scaled.
    if (mDestroyed)
    {
        return;
    }

    // event_inherited() - pEntity per-step motion + arena/core collision (:6).
    UpdateEntity();

    const float x = GetWorldX();
    const float y = GetWorldY();

    // --- Merge / absorb (Step_0.gml:26-105) ---
    if (!mHasAbsorber)
    {
        // Weapon sparkle FX while not yet claimed (Step_0.gml:28-38).
        if (mState == State::WEAPON && IRandom(4) == 0)
        {
            EmitWeaponSparkle();
        }

        // instance_place(x,y,oBubble) also matches the player (an oBubble child).
        // The player branch (Step_0.gml:48-50) makes the player this bubble's
        // absorber; the bubble-bubble branch (:51-89) merges mass immediately. The
        // "nothing overlapping at all" case arms allowMerge (Step_0.gml:93).
        if (gPlayer != nullptr && PlayerOverlaps())
        {
            if (mAllowMerge)
            {
                mHasAbsorber = true;
                mAbsorbAmount = std::round(mass / 2.0f);
            }
        }
        else if (BubbleComponent* other = FindOverlappingBubble())
        {
            if (mAllowMerge && other->mAllowMerge)
            {
                TransferMass(other);
            }
        }
        else
        {
            mAllowMerge = true;
        }
    }
    else
    {
        // Absorber exists - home toward the player, absorb on contact (:95-104).
        if (gPlayer == nullptr)
        {
            mHasAbsorber = false; // !instance_exists(absorber)
        }
        else
        {
            const float dist = PointDistance(x, y, gPlayer->CenterX(), gPlayer->CenterY());
            const float dir = PointDirection(x, y, gPlayer->CenterX(), gPlayer->CenterY());
            xSpd = LengthDirX(kHomeSpeed, dir);
            ySpd = LengthDirY(kHomeSpeed, dir);
            if (dist < kHomeSpeed)
            {
                Absorb();
                DestroySelf();
            }
        }
    }

    if (mDestroyed)
    {
        return;
    }

    // --- Squish (Step_0.gml:107-125): bursting when caught between an inner
    // (flipped/boss) wall and an outer (non-flipped) wall. ---
    {
        const bool outer = CircleVsOuterWalls(x, y, mDrawRadius).hit;
        bool inner = false;
        if (gBossWalls != nullptr)
        {
            for (const WallSegment& wall : *gBossWalls)
            {
                if (CircleVsWall(x, y, mDrawRadius, wall).hit)
                {
                    inner = true;
                    break;
                }
            }
        }
        if (inner && outer)
        {
            BurstBubble();
        }
    }

    if (mDestroyed)
    {
        return;
    }

    // --- Radius ease toward sqrt(mass/pi) (Step_0.gml:128) ---
    mDrawRadius = ApproachFade(mDrawRadius, RadiusFromMass(mass), 1.0f, 0.7f);

    // --- Destroy once too small (Step_0.gml:132-134) ---
    if (mass < 1.0f)
    {
        DestroySelf();
    }
}

bool BubbleComponent::PlayerOverlaps() const
{
    if (gPlayer == nullptr)
    {
        return false;
    }
    const float dist = PointDistance(GetWorldX(), GetWorldY(), gPlayer->CenterX(), gPlayer->CenterY());
    return dist < mDrawRadius + gPlayer->Radius();
}

BubbleComponent* BubbleComponent::FindOverlappingBubble()
{
    const float x = GetWorldX();
    const float y = GetWorldY();
    for (BubbleComponent* other : LiveBubbles())
    {
        if (other == this || other->mDestroyed)
        {
            continue;
        }
        const float dist = PointDistance(x, y, other->GetWorldX(), other->GetWorldY());
        if (dist < mDrawRadius + other->mDrawRadius)
        {
            return other;
        }
    }
    return nullptr;
}

void BubbleComponent::TransferMass(BubbleComponent* other)
{
    // Step_0.gml:51-89. Neither party is the player here (the player branch is
    // handled before this), so object_index != oPlayer throughout.
    const float dist = PointDistance(GetWorldX(), GetWorldY(), other->GetWorldX(), other->GetWorldY());
    const float overlap = (mDrawRadius + other->mDrawRadius + 2.0f) - dist;

    // min(max(overlap, 20), mass, other.mass) (Step_0.gml:59).
    float absorption = Math::Min(Math::Max(overlap, 20.0f), Math::Min(mass, other->mass));

    // Bigger bubble absorbs the smaller; tie/loss flips the sign (Step_0.gml:66).
    absorption *= (mass >= other->mass) ? 1.0f : -1.0f;

    const float absorptionRatio = absorption / mass;
    const float otherAbsorptionRatio = absorption / other->mass;

    // Update mass (Step_0.gml:69-70).
    mass += absorption;
    other->mass -= absorption;

    // Velocity blend (Step_0.gml:73-79). GML order: this updates first, then the
    // other party blends toward the ALREADY-updated velocity.
    xSpd = Math::Lerp(xSpd, other->xSpd, absorptionRatio / 2.0f);
    ySpd = Math::Lerp(ySpd, other->ySpd, absorptionRatio / 2.0f);
    other->xSpd = Math::Lerp(other->xSpd, xSpd, otherAbsorptionRatio / 2.0f);
    other->ySpd = Math::Lerp(other->ySpd, ySpd, otherAbsorptionRatio / 2.0f);

    // Destroy the under-mass party/parties (Step_0.gml:81-89). Neither is the
    // player, so the absorber-credit branch (:83-87) does not fire.
    if (mass < 1.0f)
    {
        DestroySelf();
    }
    if (other->mass < 1.0f)
    {
        other->DestroySelf();
    }
}

void BubbleComponent::Absorb()
{
    // absorb() (Step_0.gml:8-24). The absorber is always the player in this port.
    if (gPlayer == nullptr)
    {
        return;
    }

    // Score popup above the absorber's head (Step_0.gml:11-14). The amount shown
    // is the PRE-triple value; the popup's double flag renders the "x3" suffix.
    const int popupAmount = static_cast<int>(std::round(mAbsorbAmount));
    SpawnScorePopup(popupAmount,
                    mState == State::DOUBLE_POINTS,
                    /*negative=*/false,
                    gPlayer->CenterX(),
                    gPlayer->CenterY() - gPlayer->Radius() - 1.0f);

    // DOUBLE_POINTS triples the credited score (Step_0.gml:15).
    float credited = mAbsorbAmount;
    if (mState == State::DOUBLE_POINTS)
    {
        credited *= 3.0f;
    }

    // WEAPON grants the player a fireball (Step_0.gml:16-20).
    if (mState == State::WEAPON)
    {
        gPlayer->RequestFireball();
    }

    // global.score += absorbAmount (Step_0.gml:21) - routed to the flow's score.
    if (gScoreSink)
    {
        gScoreSink(static_cast<int>(std::round(credited)));
    }

    // absorber.pulse = 1; absorber.mass += mass (Step_0.gml:22-23).
    gPlayer->AddPulse(1.0f);
    gPlayer->AddMass(mass);
}

void BubbleComponent::EmitWeaponSparkle()
{
    // Step_0.gml:28-38. A 1-in-5 chance per step to emit a sparkle at a random
    // point inside the bubble. (GM also seeds the sparkle's speed/direction from
    // this bubble's velocity; SparkleComponent has no drift, so that is dropped -
    // the sparkle is a brief static flash.)
    const float x = GetWorldX();
    const float y = GetWorldY();
    const float dist = RandomRange(0.0f, mDrawRadius); // random(radius)
    const float dir = RandomRange(0.0f, 360.0f);
    const float px = x + LengthDirX(dist, dir);
    const float py = y + LengthDirY(dist, dir);

    GameObject* sparkle =
        GetOwner().GetWorld().CreateGameObject("sparkle", "Assets/Templates/Objects/CriticalCore/sparkle.json");
    if (sparkle == nullptr)
    {
        return;
    }
    if (TransformComponent* transform = sparkle->GetComponent<TransformComponent>())
    {
        transform->position.x = px;
        transform->position.y = py;
        transform->position.z = 0.0f;
    }
    sparkle->Initialize();
}

void BubbleComponent::BurstBubble()
{
    // GameOver/GameOver.gml:79-91. Spray gray trail particles, then destroy.
    if (mDestroyed)
    {
        return;
    }

    GameWorld& world = GetOwner().GetWorld();
    const float x = GetWorldX();
    const float y = GetWorldY();
    const float r = mDrawRadius;

    // repeat(max(10, mass / 50)) (GameOver.gml:81).
    const int count = static_cast<int>(Math::Max(10.0f, mass / 50.0f));
    for (int i = 0; i < count; ++i)
    {
        const float dir = RandomRange(0.0f, 360.0f);   // random(360)
        const float len = RandomRange(0.0f, r * 0.8f); // random(radius * 0.8)
        const float px = x + LengthDirX(len, dir);
        const float py = y + LengthDirY(len, dir);

        GameObject* trail =
            world.CreateGameObject("trail", "Assets/Templates/Objects/CriticalCore/trail.json");
        if (trail == nullptr)
        {
            continue;
        }
        if (TransformComponent* transform = trail->GetComponent<TransformComponent>())
        {
            transform->position.x = px;
            transform->position.y = py;
            transform->position.z = 0.0f;
        }
        if (TrailComponent* particle = trail->GetComponent<TrailComponent>())
        {
            particle->SetRadius(RandomRange(r / 4.0f, r / 3.0f)); // random_range(radius/4, radius/3)
            particle->SetColor(Graphics::Color(kGray, kGray, kGray, 1.0f)); // c_gray
            particle->SetOutline(false);                                    // filled disc
            // FadeSpeed left at the default random roll; no drift (GameOver.gml:85-87).
        }
        trail->Initialize();
    }

    DestroySelf(); // instance_destroy() (GameOver.gml:89).
}

void BubbleComponent::SpawnScorePopup(int amount, bool isDouble, bool negative, float x, float y)
{
    // ScoreComponent spawn recipe (task 28): create from score.json, position it,
    // push amount/double/negative, then Initialize. The popup VISUAL is owned by
    // ScoreComponent - the bubble never draws score text itself.
    GameObject* popup =
        GetOwner().GetWorld().CreateGameObject("score", "Assets/Templates/Objects/CriticalCore/score.json");
    if (popup == nullptr)
    {
        return;
    }
    if (TransformComponent* transform = popup->GetComponent<TransformComponent>())
    {
        transform->position.x = x;
        transform->position.y = y;
        transform->position.z = 0.0f;
    }
    if (ScoreComponent* score = popup->GetComponent<ScoreComponent>())
    {
        score->SetAmount(amount);
        score->SetDouble(isDouble);
        score->SetNegative(negative);
    }
    popup->Initialize();
}

void BubbleComponent::DestroySelf()
{
    if (mDestroyed)
    {
        return;
    }
    mDestroyed = true;
    // Deferred (ProcessDestroyList) - safe to call mid-Update; Draw guards on the
    // flag so a destroyed bubble is not drawn again this frame.
    GetOwner().GetWorld().DestroyGameObject(GetOwner().GetHandle());
}

void BubbleComponent::Draw(Render2D& render2D)
{
    if (mDestroyed)
    {
        return;
    }

    float x = 0.0f;
    float y = 0.0f;
    GetDrawPosition(x, y);

    if (mDrawRadius <= 0.0f)
    {
        return;
    }

    // oBubble/Draw_0.gml: tint by state, then drawCircle (filled disc + the
    // desaturated 1px outline = Render2D's outline=true bubble branch).
    Graphics::Color color = kColorNormal;
    switch (mState)
    {
    case State::DOUBLE_POINTS:
        color = kColorDouble;
        break;
    case State::WEAPON:
        color = kColorWeapon;
        break;
    case State::NORMAL:
    default:
        color = kColorNormal;
        break;
    }

    render2D.DrawCircleFilled(x, y, mDrawRadius, color, /*outline=*/true);
}

void BubbleComponent::Deserialize(const rapidjson::Value& value)
{
    // Base: "Depth" (Render2DComponent) + "Mass"/"SpdMult" (EntityComponent).
    EntityComponent::Deserialize(value);

    int stateInt = static_cast<int>(mState);
    if (SaveUtil::ReadInt("State", stateInt, value))
    {
        mState = static_cast<State>(stateInt);
    }
}
} // namespace Engine::CriticalCore
