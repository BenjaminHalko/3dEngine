#include "GameState.h"

using namespace Engine;
using namespace Engine::Graphics;
using namespace Engine::Input;
using namespace Engine::Physics;
using namespace Engine::Math;

// Cinematic shot timing constants
static constexpr float SHOT1_START = 0.0f;
static constexpr float SHOT2_START = 3.0f;
static constexpr float SHOT3_START = 6.0f;
static constexpr float SHOT4_START = 8.0f;
static constexpr float SHOT5_START = 9.5f;
static constexpr float SHOT6_START = 11.0f;
static constexpr float SHOT7_START = 13.0f;
static constexpr float SHOT8_START = 15.0f;
static constexpr float SHOT9_START = 17.0f;
static constexpr float CINEMATIC_END = 21.0f;

void GameState::Initialize()
{
    mCamera.SetPosition({0.0f, 1.2f, -1.5f});
    mCamera.SetLookAt({0.0f, 1.0f, 0.0f});

    mDirectionalLight.direction = Math::Normalize({1.0f, -1.0f, 1.0f});
    mDirectionalLight.ambient = {0.4f, 0.4f, 0.4f, 1.0f};
    mDirectionalLight.diffuse = {0.8f, 0.8f, 0.8f, 1.0f};
    mDirectionalLight.specular = {0.9f, 0.9f, 0.9f, 1.0f};

    // Stanley skeleton model
    mStanley.Initialize("stanley/stanley.model");
    mStanley.transform.position = {0.0f, 0.0f, 0.0f};
    mStanley.animator = &mStanleyAnimator;
    ModelManager::Get()->AddAnimation(mStanley.modelId, "Assets/Models/stanley/stanley.animset");
    mStanleyAnimator.Initialize(mStanley.modelId);

    // Ground plane
    Mesh groundMesh = MeshBuilder::CreatePlane(20, 20, 1.0f, true);
    mGround.meshBuffer.Initialize(groundMesh);
    TextureManager* tm = TextureManager::Get();
    mGround.diffuseMapId = tm->LoadTexture("terrain/grass_2048.jpg");

    // Jet model
    mPlane.Initialize("Plane/APJetFly.model");

    // Flight path animation (loops every 6s)
    const float flightDuration = 6.0f;
    mPlaneAnimTime = 0.0f;
    mPlaneFlightAnimation =
        AnimationBuilder()
            .AddPositionKey({-20.0f, 8.0f, 5.0f}, 0.0f)
            .AddPositionKey({0.0f, 8.0f, 0.0f}, flightDuration * 0.5f)
            .AddPositionKey({20.0f, 8.0f, -5.0f}, flightDuration)
            .AddRotationKey(
                Quaternion::CreateFromYawPitchRoll(-Math::Constants::Pi * 0.5f, 0.0f, 0.0f), 0.0f)
            .AddRotationKey(
                Quaternion::CreateFromYawPitchRoll(-Math::Constants::Pi * 0.5f, 0.0f, 0.0f),
                flightDuration)
            .AddScaleKey({0.3f, 0.3f, 0.3f}, 0.0f)
            .AddScaleKey({0.3f, 0.3f, 0.3f}, flightDuration)
            .Build();

    // Explosion particle system
    mParticleSystemEffect.Initialize();
    mParticleSystemEffect.SetCamera(mCamera);

    ParticleSystemInfo explosionInfo;
    explosionInfo.textureId = tm->LoadTexture("Images/explosion.png");
    explosionInfo.maxParticles = 500;
    explosionInfo.particlesPerEmit = {10, 250};
    explosionInfo.delay = 0.0f;
    explosionInfo.lifeTime = 0.0f;
    explosionInfo.timeBetweenEmit = {0.01f, 0.05f};
    explosionInfo.spawnAngle = {-180.0f, 180.0f};
    explosionInfo.spawnSpeed = {7.0f, 25.0f};
    explosionInfo.particleLifeTime = {0.5f, 2.0f};
    explosionInfo.spawnDirection = Math::Vector3::YAxis;
    explosionInfo.spawnPosition = Math::Vector3::Zero;
    explosionInfo.startScale = {Math::Vector3::One, Math::Vector3::One * 1.5f};
    explosionInfo.endScale = {Math::Vector3::One * 0.1f, Math::Vector3::One * 0.3f};
    explosionInfo.startColour = {Colors::OrangeRed, Colors::LightYellow};
    explosionInfo.endColour = {Colors::LightGray, Colors::White};
    mExplosion.Initialize(explosionInfo);

    // StandardEffect
    std::filesystem::path shaderFile = "Assets/Shaders/Standard.hlsl";
    mStandardEffect.Initialize(shaderFile);
    mStandardEffect.SetCamera(mCamera);
    mStandardEffect.SetDirectionalLight(mDirectionalLight);

    // Cinematic state
    mCinematicTime = 0.0f;
    mCinematicDone = false;
    mCurrentShot = 1;
}

void GameState::Terminate()
{
    mExplosion.Terminate();
    mParticleSystemEffect.Terminate();
    mPlane.Terminate();
    mGround.Terminate();
    mStanley.Terminate();
    mStandardEffect.Terminate();
}

void GameState::Update(float deltaTime)
{
    mStanleyAnimator.Update(deltaTime);

    // Advance plane animation (always loops)
    mPlaneAnimTime += deltaTime;
    if (mPlaneAnimTime > mPlaneFlightAnimation.GetDuration())
        mPlaneAnimTime -= mPlaneFlightAnimation.GetDuration();

    // Smooth flight transform (no shake) — used for camera and particles
    Transform smoothT = mPlaneFlightAnimation.GetTransform(mPlaneAnimTime);

    // Shot 7: override smooth position to dive straight toward Stanley
    if (mCurrentShot == 7)
    {
        float shotT = mCinematicTime - SHOT7_START;
        float t = Math::Clamp(shotT / (SHOT8_START - SHOT7_START), 0.0f, 1.0f);
        smoothT.position = Math::Lerp(mDiveStartPos, Math::Vector3{0.0f, 1.5f, 1.0f}, t);
    }

    // Apply shaking on top for rendered plane only
    Transform planeT = smoothT;
    if (mPlaneShaking)
    {
        mShakeTime += deltaTime;
        planeT.position.y += sinf(mShakeTime * 30.0f) * 0.15f;
        planeT.position.x += sinf(mShakeTime * 25.0f) * 0.08f;
    }
    mPlane.transform = planeT;

    // Advance cinematic timer
    if (!mCinematicDone)
        mCinematicTime += deltaTime;
    if (mCinematicTime >= CINEMATIC_END)
        mCinematicDone = true;

    // Determine current shot (highest threshold crossed)
    int prevShot = mCurrentShot;
    if (mCinematicTime >= SHOT9_START)
        mCurrentShot = 9;
    else if (mCinematicTime >= SHOT8_START)
        mCurrentShot = 8;
    else if (mCinematicTime >= SHOT7_START)
        mCurrentShot = 7;
    else if (mCinematicTime >= SHOT6_START)
        mCurrentShot = 6;
    else if (mCinematicTime >= SHOT5_START)
        mCurrentShot = 5;
    else if (mCinematicTime >= SHOT4_START)
        mCurrentShot = 4;
    else if (mCinematicTime >= SHOT3_START)
        mCurrentShot = 3;
    else if (mCinematicTime >= SHOT2_START)
        mCurrentShot = 2;
    else
        mCurrentShot = 1;

    // Fire one-time entry events on shot change
    if (mCurrentShot != prevShot)
        OnShotEnter(mCurrentShot, smoothT);

    // Per-shot camera assignment
    switch (mCurrentShot)
    {
    case 1:
        mCamera.SetPosition({0.0f, 1.8f, -2.5f});
        mCamera.SetLookAt({0.0f, 1.0f, 0.0f});
        break;
    case 2:
        mCamera.SetPosition({0.0f, 2.0f, -10.0f});
        mCamera.SetLookAt({0.0f, 3.0f, 0.0f});
        break;
    case 3:
    case 4:
    case 5:
        mCamera.SetPosition(smoothT.position + Math::Vector3{0.0f, 0.5f, -4.0f});
        mCamera.SetLookAt(smoothT.position);
        break;
    case 6:
        mCamera.SetPosition({0.0f, 1.2f, -2.0f});
        mCamera.SetLookAt({0.0f, 1.0f, 0.0f});
        break;
    case 7:
        mCamera.SetPosition(smoothT.position + Math::Vector3{0.0f, 2.0f, -5.0f});
        mCamera.SetLookAt(smoothT.position);
        break;
    case 8:
        mCamera.SetPosition({-5.0f, 3.0f, -8.0f});
        mCamera.SetLookAt({0.0f, 1.5f, 0.0f});
        break;
    case 9:
        mCamera.SetPosition(smoothT.position + Math::Vector3{-1.5f, 2.0f, -3.5f});
        mCamera.SetLookAt(smoothT.position + Math::Vector3{0.0f, 0.5f, 0.0f});
        mStanley.transform.position = smoothT.position + Math::Vector3{0.0f, 0.5f, 0.0f};
        break;
    }

    mStandardEffect.SetCamera(mCamera);
    mParticleSystemEffect.SetCamera(mCamera);

    // Particle system update
    if (mExplosion.IsActive())
    {
        mExplosion.SetPositon(smoothT.position);
        mExplosion.Update(deltaTime);
    }
}

void GameState::OnShotEnter(int shot, const Engine::Graphics::Transform& smoothT)
{
    switch (shot)
    {
    case 7:
        mDiveStartPos = smoothT.position;
        break;
    case 4:
        mExplosion.SetPositon(smoothT.position);
        mExplosion.SpawnParticles();
        break;
    case 5:
        mPlaneShaking = true;
        break;
    case 6:
        mPlaneShaking = false;
        mShakeTime = 0.0f;
        break;
    case 9:
        mPlaneShaking = false;
        mShakeTime = 0.0f;
        mStanley.transform.position = smoothT.position + Math::Vector3{0.0f, 0.5f, 0.0f};
        mStanleyAnimator.PlayAnimation(0, true);
        break;
    default:
        break;
    }
}

void GameState::Render()
{
    mStandardEffect.Begin();
    mStandardEffect.Render(mStanley);
    mStandardEffect.Render(mGround);
    mStandardEffect.Render(mPlane);
    mStandardEffect.End();

    mParticleSystemEffect.Begin();
    mExplosion.Render(mParticleSystemEffect);
    mParticleSystemEffect.End();
}

void GameState::DebugUI()
{
    ImGui::Begin("Cinematic", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Shot: %d / 9", mCurrentShot);
    ImGui::Text("Time: %.1f / %.1f", mCinematicTime, CINEMATIC_END);
    ImGui::Text("Done: %s", mCinematicDone ? "YES" : "NO");
    ImGui::Text("Shaking: %s", mPlaneShaking ? "YES" : "NO");
    ImGui::End();
}

void GameState::UpdateCamera(float deltaTime)
{
    // Camera is controlled by cinematic sequencer in Update()
    // Free-look kept here for future debug override (Stage 5)
    (void) deltaTime;
}
