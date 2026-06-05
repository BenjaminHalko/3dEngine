#include "SpikeComponent.h"

#include "BubbleComponent.h"
#include "Collision.h"
#include "CombatRegistry.h"
#include "CoreComponent.h"
#include "CriticalCore2DRenderService.h"
#include "GmHelpers.h"
#include "PlayerComponent.h"
#include "Render2D.h"
#include "ScoreComponent.h"

using namespace Engine;

namespace Engine::CriticalCore
{
namespace
{
// oSpike collision mask is sSpike (8x8, origin (4,4)) -> radius 4.
constexpr float kSpikeRadius = 4.0f;

// Load the snPointLoss SFX once (shared SoundId across all spikes).
Engine::Audio::SoundId PointLossSfx()
{
    static Engine::Audio::SoundId id = Engine::Audio::SoundEffectManager::Get()->Load("CriticalCore/snPointLoss.wav");
    return id;
}

// place_meeting(x, y, oCore) (oSpike/Step_0.gml:10). Uses the LIVE Core centre +
// scale when the flow published one (CombatRegistry), else the arena-centre core
// circle (consistent with EntityComponent's gate). kSpikeRadius is the sSpike mask.
bool OverlapsCore(float x, float y)
{
    float coreScale = EntityComponent::GetCoreScale();
    float coreX = Arena::kCenterX;
    float coreY = Arena::kCenterY;
    if (const CoreComponent* core = GetActiveCore())
    {
        coreX = core->CenterX();
        coreY = core->CenterY();
        coreScale = core->Scale();
    }
    const float coreRadius = Arena::kCoreSpriteRadius * coreScale;
    return PointDistance(x, y, coreX, coreY) <= coreRadius + kSpikeRadius;
}

// Spawn a negative oScore popup (score.json) at the hit point (oSpike/Step_0.gml:17-20).
void SpawnScorePopup(GameWorld& world, float x, float y, int amount)
{
    GameObject* go = world.CreateGameObject("Score", "Assets/Templates/Objects/CriticalCore/score.json");
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
    if (ScoreComponent* score = go->GetComponent<ScoreComponent>())
    {
        score->SetAmount(amount);
        score->SetNegative(true);
    }
    go->Initialize();
}
} // namespace

void SpikeComponent::Initialize()
{
    // Base caches transforms + registers with the 2D render service.
    EntityComponent::Initialize();

    // Drain the Core's launch params (initial velocity). Spikes are massless.
    CoreComponent::LaunchParams params;
    if (CoreComponent::ConsumeLaunch(&GetOwner(), params))
    {
        xSpd = params.xSpd;
        ySpd = params.ySpd;
        mass = params.mass; // 0 for spikes
    }
    // pEntity default spdMult = 0 (only oBubble sets it to 1): no core-scale push.
    spdMult = 0.0f;

    // sSpike sprite (8x8, origin (4,4)).
    if (CriticalCore2DRenderService* service = GetRenderService())
    {
        mSpikeTexture = service->GetRender2D().LoadTexture("CriticalCore/sSpike.png");
        mLoaded = true;
    }
}

void SpikeComponent::Update(float deltaTime)
{
    (void)deltaTime; // fixed-step: one Update == one GameMaker step.
    if (mDestroyed)
    {
        return;
    }

    // event_inherited(): the shared pEntity motion (reflect off walls / push out
    // of the Core) runs first (oSpike/Step_0.gml:6).
    UpdateEntity();

    // image_angle -= 10 (oSpike/Step_0.gml:8).
    mImageAngle -= 10.0f;

    const float x = GetWorldX();
    const float y = GetWorldY();

    // Guard: don't burst/damage while overlapping the Core (oSpike/Step_0.gml:10).
    if (OverlapsCore(x, y))
    {
        return;
    }

    GameWorld& world = GetOwner().GetWorld();

    // instance_place(x,y,oBubble) plain-bubble branch (Step_0.gml:11-14). The
    // bubbles publish themselves via BubbleComponent::AllBubbles() (task 23).
    for (BubbleComponent* bubble : BubbleComponent::AllBubbles())
    {
        if (bubble == nullptr)
        {
            continue;
        }
        const TransformComponent* transform = bubble->GetOwner().GetComponent<TransformComponent>();
        if (transform == nullptr)
        {
            continue;
        }
        const float bubbleRadius = EntityComponent::RadiusFromMass(bubble->GetMass());
        if (PointDistance(x, y, transform->position.x, transform->position.y) > kSpikeRadius + bubbleRadius)
        {
            continue;
        }

        bubble->BurstBubble(); // BurstBubble(_bubble) (Step_0.gml:13-14)
        mDestroyed = true;     // instance_destroy() (Step_0.gml:24)
        world.DestroyGameObject(GetOwner().GetHandle());
        return;
    }

    // instance_place player branch (Step_0.gml:15-22): oPlayer is an oBubble child.
    if (PlayerComponent* player = GetActivePlayer())
    {
        const float playerRadius = player->Radius();
        if (PointDistance(x, y, player->CenterX(), player->CenterY()) <= kSpikeRadius + playerRadius)
        {
            Engine::Audio::SoundEffectManager::Get()->Play(PointLossSfx()); // snPointLoss
            SpawnScorePopup(world, player->CenterX(), player->CenterY() - playerRadius - 1.0f, 500);
            AddScoreDelta(-500);                      // global.score -= 500
            player->SetMass(player->Mass() - 200.0f); // _bubble.mass -= 200

            mDestroyed = true; // instance_destroy() (Step_0.gml:24)
            world.DestroyGameObject(GetOwner().GetHandle());
            return;
        }
    }
}

void SpikeComponent::Draw(Render2D& render2D)
{
    if (mDestroyed || !mLoaded)
    {
        return;
    }

    float x = 0.0f;
    float y = 0.0f;
    GetDrawPosition(x, y);

    // sSpike origin (4,4); draw_sprite_ext with image_angle, scale 1, white.
    render2D.DrawSprite(mSpikeTexture, x, y, 4.0f, 4.0f, 1.0f, 1.0f, mImageAngle, Graphics::Colors::White);
}
} // namespace Engine::CriticalCore
