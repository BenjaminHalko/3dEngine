#pragma once

#include "AnimCurve.h"
#include "Collision.h"
#include "CustomTypeIds.h"
#include "Render2DComponent.h"

#include <Engine/Inc/Engine.h>

#include <array>
#include <functional>

namespace Engine::CriticalCore
{
// Forward declarations (full definitions only needed in the .cpp).
struct ShootSpawn;             // RoundConfig.h
class BeatService;             // MusicController.h
class CameraShakeService;      // CameraShakeService.h

// ---------------------------------------------------------------------------
// CoreComponent - port of GameMaker oCore (objects/oCore/*.gml).
//
// The Core is the central boss of "Critical Core 2": it sits at the arena centre
// (128,112), rotates + fires projectiles ON THE BEAT (RoundConfig::CoreShoot),
// grows/pulses each beat, heals over time, dashes around the arena along the
// CoreCurves "movement" anim curve, drives the 8 boss walls, and binds the shCore
// volumetric shader for its visual.
//
// Derives from Render2DComponent so the CriticalCore2DRenderService collects it,
// depth-sorts it, and calls Draw() into the 256x224 RenderTarget each frame.
//
// FIXED-STEP CONTRACT: Update(deltaTime) is called exactly ONCE per 60Hz fixed
// step (task 14 GameClock / task 34 GameState). deltaTime is ignored - every
// timer/speed here is in per-fixed-step units, matching the source's 60fps logic.
//
// FLOW COUPLING (task 27): the global flow state (round, gameOver, nextRound,
// roundIntro, playerHasMoved, the player position, the live weapon-projectile
// count) lives outside the Core. The flow pushes it via the setters below each
// step, and reads Hp()/NextRoundRequested() back. The Core never shoots before
// PlayerHasMoved() is true.
//
// SPAWN HAND-OFF: the Core spawns bubble/spike GameObjects via
// GameWorld::CreateGameObject(name, templatePath) and hands each its launch
// velocity + mass through the static LaunchParams registry (ConsumeLaunch),
// decoupled from the concrete Bubble/Spike components (tasks 23/24).
// ---------------------------------------------------------------------------
class CoreComponent final : public Render2DComponent
{
  public:
    SET_TYPE_ID(CustomComponentId::CoreComponent);

    // Launch parameters handed from the Core to a freshly-spawned projectile.
    // The projectile (tasks 23/24) consumes these in its OWN Initialize via
    // ConsumeLaunch(&GetOwner(), out). Keeps the Core decoupled from the
    // concrete projectile component types (it only needs CreateGameObject).
    struct LaunchParams
    {
        float xSpd = 0.0f;
        float ySpd = 0.0f;
        float mass = 0.0f;   // bubble mass; 0 for spikes
        bool weapon = false; // bubble becomes a WEAPON (purple) projectile
    };

    // Projectile consumer API (called from the projectile's Initialize). Returns
    // true + fills 'out' if the Core registered launch params for that object,
    // then removes the entry. False if none (object not Core-spawned).
    static bool ConsumeLaunch(const GameObject* gameObject, LaunchParams& out);

    // Live Core scale, mirrored to a process-global so EntityComponent (task 20)
    // and other readers can sample it without holding a Core pointer.
    static float CoreScale();

    // Live shCore animation clock (coreEffectTime), mirrored to a process-global
    // so the player + bubbles can sample it for their nebula draw without a Core
    // pointer. 0 before the Core spawns (a static nebula, harmless).
    static float EffectTime();

    void Initialize() override;
    void Terminate() override;
    void Update(float deltaTime) override;
    void DebugUI() override;
    void Deserialize(const rapidjson::Value& value) override;

    void Draw(Render2D& render2D) override;

    // Public DamageCore (CoreFunctions.gml DamageCore) - called by a fireball that
    // hits the Core (task 24) or by the flow. Applies HP/scale damage, sets the
    // heal cooldown, and on HP<=0 raises the NextRound signal.
    void DamageCore();

    // ----- Readers (task 20 entity, 27 flow, 23/24 projectiles) -----
    float Scale() const
    {
        return mScale;
    }
    float ShootDir() const
    {
        return mShootDir;
    }
    float CenterX() const
    {
        return mX;
    }
    float CenterY() const
    {
        return mY;
    }
    float Hp() const
    {
        return mHp;
    }
    int TimeSinceLastPurple() const
    {
        return mTimeSinceLastPurple;
    }
    bool PlayerHasMoved() const
    {
        return mPlayerHasMoved;
    }

    // Live boss-wall geometry: 8 center-spawned octagon segments that grow with
    // the Core (Step_0.gml:77-84). Distinct from the VISUAL nebula polygon. Task
    // 20 collision reads these for the boss-wall bounce.
    const std::array<WallSegment, 8>& BossWalls() const
    {
        return mBossWalls;
    }

    // ----- NextRound signal (read + cleared by the flow, task 27) -----
    bool NextRoundRequested() const
    {
        return mNextRoundRequested;
    }
    void ClearNextRoundRequest()
    {
        mNextRoundRequested = false;
    }
    void SetOnHpDepleted(std::function<void()> callback)
    {
        mOnHpDepleted = std::move(callback);
    }

    // ----- Flow setters (task 27 pushes these each fixed step) -----
    void SetRound(int round)
    {
        mRound = round;
    }
    void SetPlayerHasMoved(bool moved)
    {
        mPlayerHasMoved = moved;
    }
    void SetGameOver(bool gameOver)
    {
        mGameOver = gameOver;
    }
    void SetNextRound(bool nextRound)
    {
        mNextRound = nextRound;
    }
    void SetRoundIntro(bool roundIntro)
    {
        mRoundIntro = roundIntro;
    }
    void SetPlayerPosition(float x, float y)
    {
        mPlayerX = x;
        mPlayerY = y;
    }
    void SetWeaponCount(int count)
    {
        mWeaponCount = count;
    }
    void SetPulse(float pulse)
    {
        mPulse = pulse;
    }

    // Round start (GameStart RoundStart / GameStart.gml:74-93): resets shootDir,
    // flips the rotation direction, seeds the per-round targetScale, refills HP.
    void BeginRound(int round);

  private:
    void Shoot(int beatIndex);
    void SpawnProjectile(const ShootSpawn& spawn);
    void UpdateWeaponFlag(int beatIndex);
    void UpdateDash(bool shooting);
    void ProcessDashAlarm();
    void OnDashAlarm();
    void UpdateBossWalls();
    void SyncTransform();
    void DrawBossWalls(Render2D& render2D);
    void DrawCoreOctagon(
        float drawX, float drawY, float scale, float intensity, const Graphics::Color& tint, float viewScale);

    // shCore constant buffer (register b0) - layout per task 31 / the HLSL header
    // in Assets/Shaders/CriticalCore_Core.hlsl. 48 bytes = 3 float4 rows.
    struct CoreData
    {
        float iTime = 0.0f;   // row 0
        float iResX = 0.0f;
        float iResY = 0.0f;
        float iResZ = 0.0f;
        float intensity = 0.0f; // row 1
        float pad0 = 0.0f;
        float pad1 = 0.0f;
        float pad2 = 0.0f;
        float tintR = 1.0f;     // row 2 (GM draw_set_color on the shader polygon)
        float tintG = 1.0f;
        float tintB = 1.0f;
        float tintA = 1.0f;
    };
    using CoreBuffer = Graphics::TypedConstantBuffer<CoreData>;

    // Octagon nebula fan: 8 triangles * 3 verts (D3D11 has no triangle-fan).
    static constexpr int kCoreVertexCount = 24;

    // ----- Core state (oCore/Create_0.gml) -----
    float mHp = 1.0f;          // 0..1 fraction
    float mHpWaitHeal = 0.0f;  // frames before healing resumes after damage
    float mHpDraw = 1.0f;      // smoothed HP for the draw
    float mHpHit = 0.0f;       // cumulative damage taken (GetCoreIncrease term)

    float mShootDir = 0.0f;    // degrees
    bool mFlipShootDir = false;

    float mX = Arena::kCenterX;
    float mY = Arena::kCenterY;
    float mStartX = Arena::kCenterX;
    float mStartY = Arena::kCenterY;
    float mTargetX = Arena::kCenterX;
    float mTargetY = Arena::kCenterY;
    float mMovementPercent = 0.0f; // 0..1 dash progress (anim-curve input)

    int mTimeSinceLastPurple = 0;  // weapon-spawn latch (oBubble reads ==1)
    float mDashLineAmount = 0.0f;  // dash telegraph grow 0..1
    float mPulse = 0.0f;           // beat pulse, decays to 0
    float mTargetScale = 0.1f;     // per-round target scale
    float mScale = 0.0f;           // smoothed visual scale (image_xscale)
    int mAlarm0 = -1;              // dash pause alarm (-1 = inactive)

    // ----- Flow state (pushed by task 27) -----
    bool mPlayerHasMoved = false;
    bool mGameOver = false;
    bool mNextRound = false;
    bool mRoundIntro = false;
    int mRound = 1;
    int mWeaponCount = 0;
    float mPlayerX = Arena::kCenterX;
    float mPlayerY = Arena::kCenterY;

    bool mNextRoundRequested = false;
    std::function<void()> mOnHpDepleted;

    AnimCurve mMovementCurve;
    std::array<WallSegment, 8> mBossWalls{};

    TransformComponent* mTransform = nullptr; // mutable (Core dashes -> writes pos)
    BeatService* mBeatService = nullptr;
    CameraShakeService* mCameraShake = nullptr;

    // shCore visual resources.
    Graphics::VertexShader mCoreVertexShader;
    Graphics::PixelShader mCorePixelShader;
    Graphics::MeshBuffer mCoreMesh;
    CoreBuffer mCoreBuffer;
};
} // namespace Engine::CriticalCore
