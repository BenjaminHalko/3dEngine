#include "CoreComponent.h"

#include "CameraShakeService.h"
#include "GmHelpers.h"
#include "MusicController.h"
#include "Render2D.h"
#include "RoundConfig.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <unordered_map>

namespace Engine::CriticalCore
{
namespace
{
// shCore shader asset (task 31). VS/PS compiled from the same file at runtime.
const std::filesystem::path kCoreShaderPath = L"Assets/Shaders/CriticalCore_Core.hlsl";

// The Core's VISUAL octagon (oCore/Create_0.gml:55-64). DISTINCT from the
// collision wall segments - it is only a filled polygon for the nebula shader.
constexpr float kPolygonPoints[8][2] = {
    {-24.0f, -104.0f}, {24.0f, -104.0f}, {104.0f, -24.0f}, {104.0f, 24.0f},
    {24.0f, 104.0f},   {-24.0f, 104.0f}, {-104.0f, 24.0f}, {-104.0f, -24.0f}};

// GameMaker sign(): -1 / 0 / +1.
float Sign(float value)
{
    return (value > 0.0f) ? 1.0f : ((value < 0.0f) ? -1.0f : 0.0f);
}

// Pixel-space -> NDC for the shCore passthrough VS (the 256x224 RT fills NDC).
float ToNdcX(float px)
{
    return px / static_cast<float>(kInternalWidth) * 2.0f - 1.0f;
}
float ToNdcY(float py)
{
    return 1.0f - py / static_cast<float>(kInternalHeight) * 2.0f; // y-down room space
}

// Pending projectile launch params, keyed by the freshly-created GameObject. The
// Core stores an entry right before it Initialize()s the projectile; the
// projectile drains it in its own Initialize via CoreComponent::ConsumeLaunch.
std::unordered_map<const GameObject*, CoreComponent::LaunchParams> gPendingLaunches;

// Process-global mirror of the live Core scale (CoreComponent::CoreScale()).
float gCoreScale = 0.0f;

// snFireHit - a fireball striking the Core (CoreFunctions.gml:2, DamageCore).
Engine::Audio::SoundId FireHitSfx()
{
    static Engine::Audio::SoundId id =
        Engine::Audio::SoundEffectManager::Get()->Load("CriticalCore/snFireHit.wav");
    return id;
}
} // namespace

bool CoreComponent::ConsumeLaunch(const GameObject* gameObject, LaunchParams& out)
{
    const auto it = gPendingLaunches.find(gameObject);
    if (it == gPendingLaunches.end())
    {
        return false;
    }
    out = it->second;
    gPendingLaunches.erase(it);
    return true;
}

float CoreComponent::CoreScale()
{
    return gCoreScale;
}

void CoreComponent::Initialize()
{
    // Base: cache transform (const), register with the 2D render service.
    Render2DComponent::Initialize();

    // Own mutable transform pointer so the dash can write the Core position.
    mTransform = GetOwner().GetComponent<TransformComponent>();
    if (mTransform != nullptr)
    {
        mX = mTransform->position.x;
        mY = mTransform->position.y;
    }
    else
    {
        mX = Arena::kCenterX;
        mY = Arena::kCenterY;
    }
    mStartX = mX;
    mStartY = mY;
    mTargetX = mX;
    mTargetY = mY;

    // shootDir = random(360) (Create_0.gml:9).
    mShootDir = RandomRange(0.0f, 360.0f);

    // Beat + camera-shake services (null-safe; logic still runs standalone).
    mBeatService = GetOwner().GetWorld().GetService<BeatService>();
    mCameraShake = GetOwner().GetWorld().GetService<CameraShakeService>();

    // Build the 8 boss walls at spawn (center, length 0).
    UpdateBossWalls();

    gCoreScale = mScale;

    // shCore visual resources (mirrors BalatroEffect's VS/PS/cbuffer pattern).
    mCoreVertexShader.Initialize<Graphics::VertexPX>(kCoreShaderPath);
    mCorePixelShader.Initialize(kCoreShaderPath);
    mCoreBuffer.Initialize();
    mCoreMesh.Initialize(
        nullptr, static_cast<uint32_t>(sizeof(Graphics::VertexPX)), static_cast<uint32_t>(kCoreVertexCount));
    mCoreMesh.SetTopology(Graphics::MeshBuffer::Topology::Triangles);
}

void CoreComponent::Terminate()
{
    mCoreBuffer.Terminate();
    mCoreMesh.Terminate();
    mCorePixelShader.Terminate();
    mCoreVertexShader.Terminate();

    mTransform = nullptr;
    mBeatService = nullptr;
    mCameraShake = nullptr;

    // Base: unregister from the render service.
    Render2DComponent::Terminate();
}

void CoreComponent::Update(float deltaTime)
{
    (void)deltaTime; // fixed-step; all timers are per-step units.

    // GameMaker runs the Alarm phase BEFORE the Step phase.
    ProcessDashAlarm();

    const bool onBeat = (mBeatService != nullptr) && mBeatService->AudioTick() &&
                        (std::fmod(mBeatService->AudioBeat(), 1.0f) == 0.0f);
    const int beatIndex = (mBeatService != nullptr) ? mBeatService->BeatIndex() : 0;

    bool shooting = false;

    // --- Beat shoot (oCore/Step_0.gml:5-27) ---
    if (!mGameOver && !mNextRound && !mRoundIntro && mPlayerHasMoved)
    {
        if (onBeat)
        {
            mShootDir += (30.0f + mRound * 4.0f) * (mFlipShootDir ? -1.0f : 1.0f);
            UpdateWeaponFlag(beatIndex);
            Shoot(beatIndex);
        }
        shooting = true;
    }

    // --- Beat pulse + per-round growth (folded from oMusicController:44-55) ---
    if (onBeat && !mGameOver)
    {
        mPulse = 1.0f;
        if (!mNextRound && !mRoundIntro && mPlayerHasMoved)
        {
            mTargetScale += GetCoreIncrease(mRound, mHpHit);
        }
    }

    // --- Scale / pulse decay (oCore/Step_0.gml:29-34) ---
    if (!mGameOver)
    {
        mPulse = Approach(mPulse, 0.0f, 0.1f);
        mScale = ApproachFade(mScale, (mTargetScale * 208.0f + mPulse * 10.0f) / 208.0f, 1.0f, 0.7f);
    }

    // --- Heal (oCore/Step_0.gml:36-44) ---
    if (mHpWaitHeal <= 0.0f)
    {
        if (onBeat)
        {
            mHp = Math::Min(1.0f, mHp + CoreHeal());
        }
    }
    else
    {
        mHpWaitHeal = Approach(mHpWaitHeal, 0.0f, 1.0f);
    }
    mHpDraw = ApproachFade(mHpDraw, mHp, 0.1f, 0.8f);

    // --- Anim-curve dash (oCore/Step_0.gml:47-72) ---
    UpdateDash(shooting);

    // --- Dash telegraph grow (oCore/Step_0.gml:74-75) ---
    if (!mGameOver)
    {
        mDashLineAmount = ApproachFade(mDashLineAmount, 1.0f, 0.1f, 0.85f);
    }

    // --- Boss-wall scaling (oCore/Step_0.gml:77-84) ---
    UpdateBossWalls();

    SyncTransform();
    gCoreScale = mScale;
}

void CoreComponent::UpdateWeaponFlag(int beatIndex)
{
    // oCore/Step_0.gml:14-22. _count = live WEAPON bubble + fireball count, pushed
    // by the flow via SetWeaponCount (default 0 -> weapons allowed).
    if (beatIndex % 2 == 0)
    {
        --mTimeSinceLastPurple;
    }
    if (mTimeSinceLastPurple <= 0)
    {
        if (beatIndex % 2 == 0 && mWeaponCount < 3)
        {
            mTimeSinceLastPurple = 1;
        }
        else
        {
            mTimeSinceLastPurple = 0;
        }
    }
}

void CoreComponent::Shoot(int beatIndex)
{
    // Deterministic angles/counts; random kind picks resolved inside CoreShoot.
    const std::vector<ShootSpawn> spawns = CoreShoot(mRound, beatIndex);
    for (const ShootSpawn& spawn : spawns)
    {
        SpawnProjectile(spawn);
    }
}

void CoreComponent::SpawnProjectile(const ShootSpawn& spawn)
{
    // Base angle (RoundConfig.gml:36-43). aimAtPlayer resolves point_direction now.
    const float baseDir = spawn.aimAtPlayer
                              ? PointDirection(mX, mY, mPlayerX, mPlayerY)
                              : (mShootDir + spawn.angleOffset);

    // Per-shot +/-5 deg jitter on the VELOCITY direction (RoundConfig.gml:39).
    const float velDir = baseDir + RandomRange(-5.0f, 5.0f);
    const float speed = RandomRange(spawn.spdMin, spawn.spdMax);

    // Spawn 5px out from the Core along the base angle (RoundConfig.gml:38).
    const float spawnX = mX + LengthDirX(5.0f, baseDir);
    const float spawnY = mY + LengthDirY(5.0f, baseDir);

    LaunchParams params;
    params.xSpd = LengthDirX(speed, velDir);
    params.ySpd = LengthDirY(speed, velDir);
    params.mass =
        (spawn.kind == ProjectileKind::Bubble) ? RandomRange(spawn.sizeMin, spawn.sizeMax) : 0.0f;

    // First bubble of the beat consumes the weapon latch (oBubble/Create:20-24).
    params.weapon = (spawn.kind == ProjectileKind::Bubble) && (mTimeSinceLastPurple == 1);
    if (params.weapon)
    {
        mTimeSinceLastPurple = 0;
        params.mass += 10.0f; // WEAPON bubbles are heavier (RoundConfig.gml:47-48).
    }

    const char* name = (spawn.kind == ProjectileKind::Bubble) ? "Bubble" : "Spike";
    const char* templatePath = (spawn.kind == ProjectileKind::Bubble)
                                   ? "Assets/Templates/Objects/CriticalCore/bubble.json"
                                   : "Assets/Templates/Objects/CriticalCore/spike.json";

    GameObject* projectile = GetOwner().GetWorld().CreateGameObject(name, templatePath);
    if (projectile == nullptr)
    {
        return;
    }

    if (TransformComponent* transform = projectile->GetComponent<TransformComponent>())
    {
        transform->position.x = spawnX;
        transform->position.y = spawnY;
        transform->position.z = 0.0f;
    }

    // Hand the projectile its launch velocity (drained in ITS Initialize), then
    // Initialize it (CreateGameObject does NOT auto-Initialize runtime objects).
    gPendingLaunches[projectile] = params;
    projectile->Initialize();
}

void CoreComponent::ProcessDashAlarm()
{
    if (mAlarm0 > 0)
    {
        --mAlarm0;
        if (mAlarm0 == 0)
        {
            OnDashAlarm();
            mAlarm0 = -1;
        }
    }
}

void CoreComponent::OnDashAlarm()
{
    // oCore/Alarm_0.gml: pick a new dash target inside the arena, 80..200px away.
    mStartX = mX;
    mStartY = mY;

    const float spriteWidth = 208.0f * mScale; // sprite_width = base 208 * scale
    int attempts = 0;
    while (true)
    {
        ++attempts;
        if (attempts > 100)
        {
            mTargetX = Arena::kCenterX;
            mTargetY = Arena::kCenterY;
            break;
        }
        const float dist = RandomRange(0.0f, 180.0f - spriteWidth * 0.5f);
        const float dir = RandomRange(0.0f, 360.0f);
        mTargetX = LengthDirX(dist, dir) + Arena::kCenterX;
        mTargetY = LengthDirY(dist, dir) + Arena::kCenterY;
        const float d = PointDistance(mX, mY, mTargetX, mTargetY);
        if (d > 80.0f && d < 200.0f)
        {
            break;
        }
    }
    mMovementPercent = 0.0f;
    mDashLineAmount = 0.0f;
}

void CoreComponent::UpdateDash(bool shooting)
{
    if (shooting)
    {
        // Active gameplay: dash toward the random target once the telegraph is up.
        if (mAlarm0 <= 0 && mDashLineAmount > 0.99f)
        {
            mMovementPercent = Approach(mMovementPercent, 1.0f, CoreSpeed(mTargetScale));
            if (mMovementPercent >= 1.0f)
            {
                mAlarm0 = static_cast<int>(CoreSpeedPause()); // pause at the target
            }
            const float percent = mMovementCurve.Evaluate(mMovementPercent);
            mX = Math::Lerp(mStartX, mTargetX, percent);
            mY = Math::Lerp(mStartY, mTargetY, percent);
        }
    }
    else if (mRoundIntro || mNextRound)
    {
        // Between rounds / intro: ease back to the arena centre.
        mTimeSinceLastPurple = 0;
        mAlarm0 = -1;
        if (mDashLineAmount > 0.99f || mRoundIntro)
        {
            mTargetX = Arena::kCenterX;
            mTargetY = Arena::kCenterY;
            const float rate = mRoundIntro ? 0.05f : CoreSpeed(mTargetScale) * 2.0f;
            mMovementPercent = Approach(mMovementPercent, 1.0f, rate);
            const float percent = mMovementCurve.Evaluate(mMovementPercent);
            mX = Math::Lerp(mStartX, mTargetX, percent);
            mY = Math::Lerp(mStartY, mTargetY, percent);
        }
    }
    else
    {
        mAlarm0 = -1;
    }
}

void CoreComponent::UpdateBossWalls()
{
    // oCore/Step_0.gml:77-84. Each boss wall grows from the Core centre toward its
    // matching outer wall by _scale = core.scale + 0.12, scaling its length too.
    const std::array<WallSegment, 8>& outer = OuterWalls();
    const float growth = mScale + 0.12f;
    for (int i = 0; i < 8; ++i)
    {
        const float outerScaleX = outer[i].length / Arena::kWallSpriteThickness; // length/4
        const float bossScaleX = outerScaleX * growth;
        const float bossX = mX + Math::Lerp(Arena::kCenterX, outer[i].x, growth) - Arena::kCenterX;
        const float bossY = mY + Math::Lerp(Arena::kCenterY, outer[i].y, growth) - Arena::kCenterY;
        mBossWalls[i] =
            MakeWall(bossX, bossY, outer[i].angle, bossScaleX, /*flipped=*/true, /*bossWall=*/true);
    }
}

void CoreComponent::SyncTransform()
{
    if (mTransform != nullptr)
    {
        mTransform->position.x = mX;
        mTransform->position.y = mY;
    }
}

void CoreComponent::DamageCore()
{
    // CoreFunctions.gml DamageCore:2-3 - snFireHit then ScreenShake(10,20).
    // (oBackground.bgFlash is driven by its own system.)
    if (Engine::Audio::SoundEffectManager* sfx = Engine::Audio::SoundEffectManager::Get())
    {
        sfx->Play(FireHitSfx());
    }
    if (mCameraShake != nullptr)
    {
        mCameraShake->ScreenShake(10.0f, 20);
    }
    if (mTargetScale < 0.4f)
    {
        mTimeSinceLastPurple = 3;
        mShootDir = RandomRange(0.0f, 360.0f);
    }
    mHp = Math::Max(0.0f, mHp - CoreHPDamage(mRound));
    mTargetScale -= CoreTotalDamage(mHp, mTargetScale);
    mHpWaitHeal = CoreWaitToHeal();
    mHpHit += CoreHPDamage(mRound);

    if (mHp <= 0.0f)
    {
        mNextRoundRequested = true;
        if (mOnHpDepleted)
        {
            mOnHpDepleted();
        }
    }
}

void CoreComponent::BeginRound(int round)
{
    // GameStart RoundStart (GameStart.gml:74-93).
    mRound = round;
    mPlayerHasMoved = false;
    mShootDir = 0.0f;
    mFlipShootDir = !mFlipShootDir;
    mTargetScale = GetCoreStart(round);
    mHp = 1.0f;
    mHpHit = 0.0f;

    // Reset the dash to ease back to centre (GameOver/NextRound reset semantics).
    mStartX = mX;
    mStartY = mY;
    mDashLineAmount = 0.0f;
    mMovementPercent = 0.0f;
    mAlarm0 = -1;
}

void CoreComponent::Draw(Render2D& render2D)
{
    float drawX = 0.0f;
    float drawY = 0.0f;
    GetDrawPosition(drawX, drawY); // Core centre in draw space (camera applied)

    // --- Dash-line telegraph (oCore/Draw_0.gml:3-9) ---
    if (mDashLineAmount > 0.0f && mMovementPercent < 1.0f)
    {
        const float dist = PointDistance(mX, mY, mTargetX, mTargetY) * mDashLineAmount;
        const float dir = PointDirection(mX, mY, mTargetX, mTargetY);
        float endX = mX + LengthDirX(dist, dir);
        float endY = mY + LengthDirY(dist, dir);
        ApplyCameraOffset(endX, endY);
        render2D.DrawLine(drawX, drawY, endX, endY, 1.0f, Graphics::Colors::White);
    }

    // --- shCore nebula octagon (oCore/Draw_0.gml:11-27) ---
    if (mHpDraw < 0.995f)
    {
        DrawCoreShader(drawX, drawY);
    }

    // --- HP overlay disc (oCore/Draw_0.gml:29-43) ---
    if (mHpDraw > 0.01f)
    {
        float hpScale = mScale * mHpDraw;
        if (mHp != 1.0f && mHp != 0.0f)
        {
            hpScale = Math::Max(0.05f, hpScale);
        }
        const float hpRadius = Arena::kCoreSpriteRadius * hpScale;
        const Graphics::Color hpBase = {1.0f, 0.0f, 94.0f / 255.0f, 1.0f}; // #FF005E
        const Graphics::Color hpColor = MergeColor(hpBase, Graphics::Colors::Red, mPulse);
        render2D.DrawCircleFilled(drawX, drawY, hpRadius, hpColor);
    }
}

void CoreComponent::DrawCoreShader(float drawX, float drawY)
{
    // Build the filled nebula octagon as a triangle fan (center, p_i, p_{i+1}).
    // Positions are NDC (shCore VS is a passthrough); uv = world / internal res,
    // matching GM's draw_vertex_texture(x, y, x/RES_WIDTH, y/RES_HEIGHT).
    std::array<Graphics::VertexPX, kCoreVertexCount> verts{};

    const float centerNdcX = ToNdcX(drawX);
    const float centerNdcY = ToNdcY(drawY);
    const Math::Vector2 centerUv = {mX / static_cast<float>(kInternalWidth),
                                    mY / static_cast<float>(kInternalHeight)};

    int v = 0;
    for (int i = 0; i < 8; ++i)
    {
        const int j = (i + 1) % 8;

        // perimeter point i (world, pixel-snapped like Draw_0.gml:23)
        const float wxi = mX + kPolygonPoints[i][0] * mScale - Sign(kPolygonPoints[i][0]);
        const float wyi = mY + kPolygonPoints[i][1] * mScale - Sign(kPolygonPoints[i][1]);
        float dxi = wxi;
        float dyi = wyi;
        ApplyCameraOffset(dxi, dyi);

        // perimeter point j
        const float wxj = mX + kPolygonPoints[j][0] * mScale - Sign(kPolygonPoints[j][0]);
        const float wyj = mY + kPolygonPoints[j][1] * mScale - Sign(kPolygonPoints[j][1]);
        float dxj = wxj;
        float dyj = wyj;
        ApplyCameraOffset(dxj, dyj);

        verts[v++] = {{centerNdcX, centerNdcY, 0.0f}, centerUv};
        verts[v++] = {{ToNdcX(dxi), ToNdcY(dyi), 0.0f},
                      {wxi / static_cast<float>(kInternalWidth), wyi / static_cast<float>(kInternalHeight)}};
        verts[v++] = {{ToNdcX(dxj), ToNdcY(dyj), 0.0f},
                      {wxj / static_cast<float>(kInternalWidth), wyj / static_cast<float>(kInternalHeight)}};
    }

    mCoreMesh.Update(verts.data(), static_cast<uint32_t>(kCoreVertexCount));

    CoreData data;
    // GM binds uTime = -coreEffectTime (Draw_0.gml:12).
    data.iTime = (mBeatService != nullptr) ? -mBeatService->CoreEffectTime() : 0.0f;
    data.iResX = static_cast<float>(kInternalWidth);  // 256
    data.iResY = static_cast<float>(kInternalHeight); // 224
    data.iResZ = 0.0f;
    // GM nebula intensity baseline is 0.5; we drive it with the beat pulse so the
    // nebula brightens on the beat (intensity "from the pulse").
    data.intensity = 0.5f + Math::Clamp(mPulse, 0.0f, 1.0f) * 0.5f;
    data.pad0 = 0.0f;
    data.pad1 = 0.0f;
    data.pad2 = 0.0f;

    mCoreVertexShader.Bind();
    mCorePixelShader.Bind();
    mCoreBuffer.Update(data);
    mCoreBuffer.BindPS(0);
    mCoreMesh.Render();
}

void CoreComponent::DebugUI()
{
    ImGui::Text("Core HP: %.2f  Scale: %.3f", mHp, mScale);
    ImGui::Text("ShootDir: %.1f  Round: %d", mShootDir, mRound);
    ImGui::Text("Pos: (%.1f, %.1f)  Dash%%: %.2f", mX, mY, mMovementPercent);
    ImGui::Text("PlayerMoved: %d  NextRound: %d", mPlayerHasMoved ? 1 : 0, mNextRoundRequested ? 1 : 0);
}

void CoreComponent::Deserialize(const rapidjson::Value& value)
{
    Render2DComponent::Deserialize(value); // reads "Depth"

    if (value.HasMember("Round"))
    {
        mRound = value["Round"].GetInt();
    }
    if (value.HasMember("TargetScale"))
    {
        mTargetScale = value["TargetScale"].GetFloat();
    }
}
} // namespace Engine::CriticalCore
