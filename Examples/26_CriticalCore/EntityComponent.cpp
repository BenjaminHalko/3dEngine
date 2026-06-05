#include "EntityComponent.h"

#include "Collision.h"
#include "GmHelpers.h"

#include <cmath>

using namespace Engine;

namespace Engine::CriticalCore
{
namespace
{
    constexpr float kPi = 3.14159265358979323846f;
} // namespace

float EntityComponent::sCoreScale = 0.0f;

void EntityComponent::Initialize()
{
    // Base caches the CONST transform (for drawing) + registers with the render
    // service. We additionally cache a MUTABLE transform to integrate position.
    Render2DComponent::Initialize();
    mEntityTransform = GetOwner().GetComponent<TransformComponent>();
}

void EntityComponent::Terminate()
{
    mEntityTransform = nullptr;
    Render2DComponent::Terminate();
}

void EntityComponent::Deserialize(const rapidjson::Value& value)
{
    // Pull Render2DComponent's "Depth", then the shared pEntity tuning fields.
    Render2DComponent::Deserialize(value);
    SaveUtil::ReadFloat("Mass", mass, value);
    SaveUtil::ReadFloat("SpdMult", spdMult, value);
}

void EntityComponent::SetCoreScale(float scale)
{
    sCoreScale = scale;
}

float EntityComponent::GetCoreScale()
{
    return sCoreScale;
}

float EntityComponent::RadiusFromMass(float mass)
{
    if (mass <= 0.0f)
    {
        return 0.0f;
    }
    return std::sqrt(mass / kPi);
}

float EntityComponent::Radius() const
{
    return RadiusFromMass(mass);
}

void EntityComponent::UpdateEntity()
{
    float x = (mEntityTransform != nullptr) ? mEntityTransform->position.x : 0.0f;
    float y = (mEntityTransform != nullptr) ? mEntityTransform->position.y : 0.0f;

    const float radius = CollisionRadius();
    const bool isPlayer = IsPlayer();

    // place_meeting(x,y,oWall) predicate for the pixel-step marches. Uses the
    // canonical outer-wall geometry from Collision.h (no duplicated math).
    const auto inWall = [radius](float px, float py) -> bool {
        return CircleVsOuterWalls(px, py, radius).hit;
    };

    if (collide)
    {
        // instance_place(x+xSpd, y+ySpd, oWall) (Step_0.gml:5).
        const WallHit wall = CircleVsOuterWalls(x + xSpd, y + ySpd, radius);
        if (wall.hit)
        {
            const float dir = PointDirection(0.0f, 0.0f, xSpd, ySpd);

            // Boss-wall push-out for non-player bodies (Step_0.gml:9-17). NOTE:
            // CircleVsOuterWalls only reports the 8 OUTER walls (bossWall=false),
            // so this branch stays dormant until the live boss walls are wired
            // into the entity query (Core, task 21). Kept faithful to the source.
            if (wall.bossWall && !isPlayer)
            {
                MarchIntoContact(x, y, dir, inWall);
            }

            // Reflect gate: angle_difference <= 90 (Step_0.gml:19).
            if (ShouldReflect(dir, wall.wallAngle, wall.flipped))
            {
                if (!isPlayer)
                {
                    // Non-player: clamp+mirror reflect, then re-derive velocity
                    // from the same speed magnitude (Step_0.gml:20-30).
                    const float len = PointDistance(0.0f, 0.0f, xSpd, ySpd);
                    const float reflected = ReflectOffWall(dir, wall.wallAngle, wall.flipped);
                    xSpd = LengthDirX(len, reflected);
                    ySpd = LengthDirY(len, reflected);
                }
                else
                {
                    // Player: march into contact, then stop dead (Step_0.gml:31-41).
                    MarchIntoContact(x, y, dir, inWall);
                    xSpd = 0.0f;
                    ySpd = 0.0f;
                }
            }

            // Player-vs-wall death (Step_0.gml:44-48). Needs player-only state
            // (playerHasMoved, deathDelay), so PlayerComponent (task 22) owns it
            // via this hook; non-player bodies never reach here.
            if (isPlayer)
            {
                OnWallTouched(wall);
            }
        }
        else
        {
            // place_meeting(x,y,oCore): non-player bodies get a radial push-out
            // off the core (Step_0.gml:49-54); the PLAYER instead dies on contact
            // (user-requested "the core should kill you").
            const CoreHit core = CircleVsCore(x, y, radius, sCoreScale);
            if (core.hit)
            {
                if (!isPlayer)
                {
                    const float dir = CoreRedirectDir(x, y);
                    const float len = PointDistance(0.0f, 0.0f, xSpd, ySpd);
                    xSpd = LengthDirX(len, dir);
                    ySpd = LengthDirY(len, dir);
                }
                else
                {
                    OnCoreTouched();
                }
            }
        }
    }
    else if (!CircleVsCore(x, y, radius, sCoreScale).hit)
    {
        // "Don't collide until outside the core" gate (Step_0.gml:55-56): bodies
        // spawn inside the core and only arm collision once they have cleared it.
        collide = true;
    }

    // Move (Step_0.gml:59-64). spdMult fades to 0 once colliding; the motion is
    // boosted by the live core scale. NO dt - this runs inside the fixed step.
    if (collide)
    {
        spdMult = ApproachFade(spdMult, 0.0f, 0.1f, 0.8f);
    }
    x += xSpd * (1.0f + spdMult * sCoreScale);
    y += ySpd * (1.0f + spdMult * sCoreScale);

    if (mEntityTransform != nullptr)
    {
        mEntityTransform->position.x = x;
        mEntityTransform->position.y = y;
    }
}
} // namespace Engine::CriticalCore
