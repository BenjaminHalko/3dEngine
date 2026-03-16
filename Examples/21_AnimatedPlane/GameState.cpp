#include "GameState.h"

using namespace Engine;
using namespace Engine::Graphics;
using namespace Engine::Input;
using namespace Engine::Physics;
using namespace Engine::Math;

void GameState::Initialize()
{
    mCamera.SetPosition({0.0f, 3.0f, -10.0f});
    mCamera.SetLookAt({0.0f, 3.0f, 0.0f});

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

    // Flight path animation
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

    // Explosion particle system (fire -> smoke burst)
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
    explosionInfo.endScale   = {Math::Vector3::One * 0.1f, Math::Vector3::One * 0.3f};
    explosionInfo.startColour = {Colors::OrangeRed, Colors::LightYellow};
    explosionInfo.endColour   = {Colors::LightGray,  Colors::White};
    mExplosion.Initialize(explosionInfo);

    // StandardEffect
    std::filesystem::path shaderFile = "Assets/Shaders/Standard.hlsl";
    mStandardEffect.Initialize(shaderFile);
    mStandardEffect.SetCamera(mCamera);
    mStandardEffect.SetDirectionalLight(mDirectionalLight);
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
    UpdateCamera(deltaTime);
    mStanleyAnimator.Update(deltaTime);

    // Advance scene timer
    mSceneTimer += deltaTime;

    // Advance plane animation (loop)
    mPlaneAnimTime += deltaTime;
    if (mPlaneAnimTime > mPlaneFlightAnimation.GetDuration())
        mPlaneAnimTime -= mPlaneFlightAnimation.GetDuration();

    // Smooth flight transform (camera tracks this -- no shake)
    Transform smoothT = mPlaneFlightAnimation.GetTransform(mPlaneAnimTime);

    // Apply shaking on top for rendered plane only
    Transform planeT = smoothT;
    if (mPlaneShaking)
    {
        mShakeTime += deltaTime;
        planeT.position.y += sinf(mShakeTime * 30.0f) * 0.15f;
        planeT.position.x += sinf(mShakeTime * 25.0f) * 0.08f;
    }

    mPlane.transform = planeT;
    // Trigger explosion at 3 seconds
    if (mSceneTimer >= 3.0f && !mExplosionTriggered)
    {
        mExplosionTriggered = true;
        mPlaneShaking = true;
        mExplosion.SetPositon(smoothT.position);
        mExplosion.SpawnParticles();
    }

    // Update particles only after triggered, track smooth position
    if (mExplosionTriggered)
    {
        mExplosion.SetPositon(smoothT.position);
        mExplosion.Update(deltaTime);
    }

    // Camera follows smooth path -- not shake
    mCamera.SetPosition(smoothT.position + Math::Vector3{0.0f, 2.0f, -8.0f});
    mCamera.SetLookAt(smoothT.position);
    mStandardEffect.SetCamera(mCamera);
    mParticleSystemEffect.SetCamera(mCamera);
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
    ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Stage 3: Explosion + Shaking");
    ImGui::Text("Scene time: %.1f", mSceneTimer);
    ImGui::Text("Shaking: %s", mPlaneShaking ? "YES" : "NO");
    ImGui::Text("Explosion: %s", mExplosionTriggered ? "TRIGGERED" : "waiting...");
    ImGui::End();
}

void GameState::UpdateCamera(float deltaTime)
{
    InputSystem* input = InputSystem::Get();
    const float moveSpeed = input->IsKeyDown(KeyCode::LSHIFT) ? 10.0f : 4.0f;
    const float turnSpeed = 0.5f;

    if (input->IsKeyDown(KeyCode::W))
        mCamera.Walk(moveSpeed * deltaTime);
    else if (input->IsKeyDown(KeyCode::S))
        mCamera.Walk(-moveSpeed * deltaTime);
    else if (input->IsKeyDown(KeyCode::D))
        mCamera.Strafe(moveSpeed * deltaTime);
    else if (input->IsKeyDown(KeyCode::A))
        mCamera.Strafe(-moveSpeed * deltaTime);
    else if (input->IsKeyDown(KeyCode::E))
        mCamera.Rise(moveSpeed * deltaTime);
    else if (input->IsKeyDown(KeyCode::Q))
        mCamera.Rise(-moveSpeed * deltaTime);

    if (input->IsMouseDown(MouseButton::RBUTTON))
    {
        mCamera.Yaw(input->GetMouseMoveX() * turnSpeed * deltaTime);
        mCamera.Pitch(input->GetMouseMoveY() * turnSpeed * deltaTime);
    }
}
