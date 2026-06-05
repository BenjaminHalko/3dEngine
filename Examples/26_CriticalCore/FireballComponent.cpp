#include "FireballComponent.h"

#include "CameraShakeService.h"
#include "Collision.h"
#include "CombatRegistry.h"
#include "CoreComponent.h"
#include "GameClock.h"
#include "GmHelpers.h"
#include "PlayerComponent.h"
#include "Render2D.h"
#include "TrailComponent.h"

#include <cmath>

using namespace Engine;

namespace Engine::CriticalCore
{
namespace
{
constexpr float kPi = 3.14159265358979323846f;

// oFireball collision mask is sPlayerMask (16x16, origin (8,8)) -> radius 8. The
// DRAWN radius (10..50, visual only) is NOT the collision size (the GML
// instance_place uses the sprite mask, not the drawCircle radius).
constexpr float kFireballCollisionRadius = 8.0f;

// oFireball sprite is sPlayerMask -> sprite_width = 16 (Step_0.gml:19 amplitude).
constexpr float kSpriteWidth = 16.0f;

// Trail tints: choose(#EE8213, #EEA612) (Step_0.gml:24 / GameOver.gml:129).
const Graphics::Color kOrange1{238.0f / 255.0f, 130.0f / 255.0f, 19.0f / 255.0f, 1.0f}; // #EE8213
const Graphics::Color kOrange2{238.0f / 255.0f, 166.0f / 255.0f, 18.0f / 255.0f, 1.0f}; // #EEA612

// Load the snFireShoot SFX once (shared SoundId across all fireballs).
Engine::Audio::SoundId FireShootSfx()
{
    static Engine::Audio::SoundId id = Engine::Audio::SoundEffectManager::Get()->Load("CriticalCore/snFireShoot.wav");
    return id;
}

Graphics::Color ChooseOrange()
{
    return (IRandom(1) == 0) ? kOrange1 : kOrange2;
}

// Spawn one oPlayerTrail particle (trail.json) and configure it (TrailComponent).
// fadeSpeed < 0 lets the trail roll its own random fade (oPlayerTrail default).
void SpawnTrail(GameWorld& world,
                float x,
                float y,
                float radius,
                float driftSpeed,
                float driftDir,
                const Graphics::Color& color,
                float fadeSpeed,
                bool outline,
                float depth)
{
    GameObject* go = world.CreateGameObject("Trail", "Assets/Templates/Objects/CriticalCore/trail.json");
    if (go == nullptr)
    {
        return;
    }
    if (TransformComponent* transform = go->GetComponent<TransformComponent>())
    {
        transform->position.x = x;
        transform->position.y = y;
        transform->position.z = 0.0f;
    }
    if (TrailComponent* trail = go->GetComponent<TrailComponent>())
    {
        trail->SetRadius(radius);
        trail->SetFadeSpeed(fadeSpeed);
        trail->SetDrift(driftSpeed, driftDir);
        trail->SetColor(color);
        trail->SetOutline(outline);
        trail->SetDepth(depth);
    }
    go->Initialize();
}
} // namespace

void FireballComponent::Initialize()
{
    // Base caches the const transform + registers with the 2D render service.
    Render2DComponent::Initialize();

    // Mutable transform so we can integrate the straight-line motion.
    mTransform = GetOwner().GetComponent<TransformComponent>();

    // Active Core (published by the flow via CombatRegistry) unless one was set.
    if (mCore == nullptr)
    {
        mCore = GetActiveCore();
    }
    mCameraShake = GetOwner().GetWorld().GetService<CameraShakeService>();

    // snFireShoot (oFireball/Create_0.gml:5).
    Engine::Audio::SoundEffectManager::Get()->Play(FireShootSfx());

    // ScreenShake(4, 10) (oFireball/Create_0.gml:7).
    if (mCameraShake != nullptr)
    {
        mCameraShake->ScreenShake(4.0f, 10);
    }

    // radius = clamp(sqrt(max(0, oPlayer.mass) / pi), 10, 50) (Create_0.gml:6). The
    // spawning Player does not pass its mass, so fall back to the live player's
    // radius (== sqrt(mass/pi)) when SetSourceMass was not used.
    float sourceRadius = std::sqrt(Math::Max(0.0f, mSourceMass) / kPi);
    if (mSourceMass <= 0.0f)
    {
        if (const PlayerComponent* player = GetActivePlayer())
        {
            sourceRadius = player->Radius();
        }
    }
    mRadius = Math::Clamp(sourceRadius, 10.0f, 50.0f);

    // dir = point_direction(x, y, oCore.x, oCore.y) (Create_0.gml:9). Falls back
    // to the arena centre when no Core is published.
    const float x = (mTransform != nullptr) ? mTransform->position.x : Arena::kCenterX;
    const float y = (mTransform != nullptr) ? mTransform->position.y : Arena::kCenterY;
    const float coreX = (mCore != nullptr) ? mCore->CenterX() : Arena::kCenterX;
    const float coreY = (mCore != nullptr) ? mCore->CenterY() : Arena::kCenterY;
    mDir = PointDirection(x, y, coreX, coreY);
}

void FireballComponent::Update(float deltaTime)
{
    (void)deltaTime; // fixed-step: one Update == one GameMaker step.
    if (mDestroyed)
    {
        return;
    }

    float x = (mTransform != nullptr) ? mTransform->position.x : 0.0f;
    float y = (mTransform != nullptr) ? mTransform->position.y : 0.0f;

    // Move toward the Core at a fixed 6 px / step (Step_0.gml:8-9).
    x += LengthDirX(6.0f, mDir);
    y += LengthDirY(6.0f, mDir);

    // instance_place(x, y, oWall): test the live boss walls AND the outer walls
    // (Step_0.gml:11-17). A boss-wall hit damages the Core; any wall ends it.
    bool bossHit = false;
    if (mCore != nullptr)
    {
        const std::array<WallSegment, 8>& bossWalls = mCore->BossWalls();
        for (const WallSegment& wall : bossWalls)
        {
            if (CircleVsWall(x, y, kFireballCollisionRadius, wall).hit)
            {
                bossHit = true;
                break;
            }
        }
    }
    const bool outerHit = CircleVsOuterWalls(x, y, kFireballCollisionRadius).hit;

    if (bossHit || outerHit)
    {
        // Commit the impact position so the burst particles spawn at the wall.
        if (mTransform != nullptr)
        {
            mTransform->position.x = x;
            mTransform->position.y = y;
        }
        if (bossHit && mCore != nullptr)
        {
            mCore->DamageCore(); // CoreFunctions.gml DamageCore (formula owned by the Core).
        }
        FireballCollect(x, y);
        mDestroyed = true;
        GetOwner().GetWorld().DestroyGameObject(GetOwner().GetHandle());
        return;
    }

    // Wavy trail (Step_0.gml:19-26): a single particle offset perpendicular to the
    // heading by a sine that sweeps +-sprite_width/2. mAge drives the Wave phase
    // (a local accumulator is equivalent to the global clock for a continuous sine).
    mAge += GameClock::kStep;
    const float waveAmount = Wave(-1.0f, 1.0f, 1.0f, 0.0f, mAge) * kSpriteWidth / 2.0f;
    const float trailX = x + LengthDirX(waveAmount, mDir + 90.0f);
    const float trailY = y + LengthDirY(waveAmount, mDir + 90.0f);
    SpawnTrail(GetOwner().GetWorld(), trailX, trailY, 5.0f, RandomRange(0.0f, 2.0f),
               RandomRange(0.0f, 360.0f), ChooseOrange(), RandomRange(0.02f, 0.05f),
               /*outline=*/false, GetDepth() - 1.0f);

    // Commit the new position.
    if (mTransform != nullptr)
    {
        mTransform->position.x = x;
        mTransform->position.y = y;
    }
}

void FireballComponent::FireballCollect(float x, float y)
{
    // GameOver.gml:118-133.
    if (mCameraShake != nullptr)
    {
        mCameraShake->ScreenShake(4.0f, 5); // GameOver.gml:119
    }

    GameWorld& world = GetOwner().GetWorld();

    // One plain trail at the impact (GameOver.gml:120) - default ltgrey outline,
    // its own random fade (fadeSpeed < 0 sentinel).
    SpawnTrail(world, x, y, 4.0f, 0.0f, 0.0f, Graphics::Color(0.75294f, 0.75294f, 0.75294f, 1.0f),
               /*fadeSpeed=*/-1.0f, /*outline=*/true, GetDepth());

    // repeat(50) orange burst particles (GameOver.gml:121-132).
    const float r = Math::Max(12.0f, mRadius);
    for (int i = 0; i < 50; ++i)
    {
        const float dir = RandomRange(0.0f, 360.0f);
        const float len = RandomRange(0.0f, r * 0.8f);
        const float px = x + LengthDirX(len, dir);
        const float py = y + LengthDirY(len, dir);
        SpawnTrail(world, px, py, RandomRange(r / 5.0f, r / 3.0f), RandomRange(0.0f, 2.0f),
                   RandomRange(0.0f, 360.0f), ChooseOrange(), RandomRange(0.02f, 0.05f),
                   /*outline=*/false, GetDepth() + 1.0f);
    }
}

void FireballComponent::Draw(Render2D& render2D)
{
    if (mDestroyed)
    {
        return;
    }

    float x = 0.0f;
    float y = 0.0f;
    GetDrawPosition(x, y);

    // oFireball/Draw_0.gml: drawCircle(x, y, radius) in image_blend (default white).
    // GameMaker drawCircle = filled disc + desaturated outline (DrawCircleFilled
    // outline branch, Render2D task 11).
    render2D.DrawCircleFilled(x, y, mRadius, Graphics::Colors::White, /*outline=*/true);
}

void FireballComponent::Deserialize(const rapidjson::Value& value)
{
    Render2DComponent::Deserialize(value); // reads "Depth"
}
} // namespace Engine::CriticalCore
