#include "GameState.h"

using namespace Engine;
using namespace Engine::Graphics;
using namespace Engine::Input;
using namespace Engine::Physics;
using namespace Engine::Math;
using namespace Engine::Audio;

void GameState::Initialize()
{
    mCamera.SetPosition({0.0f, 1.2f, -1.5f});
    mCamera.SetLookAt({0.0f, 1.0f, 0.0f});

    mDirectionalLight.direction = Math::Normalize({1.0f, -1.0f, 1.0f});
    mDirectionalLight.ambient = {0.4f, 0.4f, 0.4f, 1.0f};
    mDirectionalLight.diffuse = {0.8f, 0.8f, 0.8f, 1.0f};
    mDirectionalLight.specular = {0.9f, 0.9f, 0.9f, 1.0f};

    mStanley.Initialize("stanley/stanley.model");
    mStanley.transform.position = {0.0f, 0.0f, 0.0f};
    mStanley.animator = &mStanleyAnimator;
    ModelManager::Get()->AddAnimation(mStanley.modelId, "Assets/Models/stanley/stanley.animset");
    ModelManager::Get()->AddAnimation(mStanley.modelId, "Assets/Models/stanley/salute.animset");
    mStanleyAnimator.Initialize(mStanley.modelId);

    Mesh groundMesh = MeshBuilder::CreatePlane(20, 20, 1.0f, true);
    mGround.meshBuffer.Initialize(groundMesh);
    TextureManager* tm = TextureManager::Get();
    mGround.diffuseMapId = tm->LoadTexture("terrain/grass_2048.jpg");

    mPlane.Initialize("Plane/APJetFly.model");

    const float flightDuration = 6.0f;
    mPlaneAnimTime = 0.0f;
    mPlaneFlightAnimation =
        AnimationBuilder()
            .AddPositionKey({-20.0f, 4.0f, 5.0f}, 0.0f)
            .AddPositionKey({0.0f, 4.0f, 0.0f}, flightDuration * 0.5f)
            .AddPositionKey({20.0f, 4.0f, -5.0f}, flightDuration)
            .AddRotationKey(
                Quaternion::CreateFromYawPitchRoll(-Math::Constants::Pi * 0.5f, 0.0f, 0.0f), 0.0f)
            .AddRotationKey(
                Quaternion::CreateFromYawPitchRoll(-Math::Constants::Pi * 0.5f, 0.0f, 0.0f),
                flightDuration)
            .AddScaleKey({0.3f, 0.3f, 0.3f}, 0.0f)
            .AddScaleKey({0.3f, 0.3f, 0.3f}, flightDuration)
            .Build();

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

    std::filesystem::path shaderFile = "Assets/Shaders/Standard.hlsl";
    mStandardEffect.Initialize(shaderFile);
    mStandardEffect.SetCamera(mCamera);
    mStandardEffect.SetDirectionalLight(mDirectionalLight);

    mMusicId = SoundEffectManager::Get()->Load("dixie.mp3");
    SoundEffectManager::Get()->Play(mMusicId, true);

    mStanleyAnimator.PlayAnimation(0, true);
    ResetCinematic();
}

void GameState::Terminate()
{
    if (mGiantLoaded)
        mGiantStanley.Terminate();
    if (mTowerLoaded)
    {
        mTower.Terminate();
        mTower2.Terminate();
        mTower3.Terminate();
    }
    mExplosion.Terminate();
    mParticleSystemEffect.Terminate();
    mPlane.Terminate();
    mGround.Terminate();
    mStanley.Terminate();
    mStandardEffect.Terminate();
    SoundEffectManager::Get()->Stop(mMusicId);
}

void GameState::ResetCinematic()
{
    mCinematicTime = 0.0f;
    mCinematicDone = false;
    mCurrentShot = 1;
    mPlaneAnimTime = 0.0f;
    mPlaneShaking = false;
    mShakeTime = 0.0f;
    mStanley.transform.position = {0.0f, 0.0f, 0.0f};
    mStanleyAnimator.PlayAnimation(0, true);
    if (mGiantLoaded)
    {
        mGiantStanley.Terminate();
        mGiantLoaded = false;
    }
    if (mTowerLoaded)
    {
        mTower.Terminate();
        mTower2.Terminate();
        mTower3.Terminate();
        mTowerLoaded = false;
    }
    SoundEffectManager::Get()->Stop(mMusicId);
    SoundEffectManager::Get()->Play(mMusicId, true);
}

void GameState::Update(float deltaTime)
{
    mStanleyAnimator.Update(deltaTime);
    if (mGiantLoaded)
        mGiantAnimator.Update(deltaTime);

    if (!mPaused && !mCinematicDone)
    {
        mPlaneAnimTime += deltaTime;
        if (mPlaneAnimTime > mPlaneFlightAnimation.GetDuration())
            mPlaneAnimTime -= mPlaneFlightAnimation.GetDuration();
    }

    Transform smoothT = mPlaneFlightAnimation.GetTransform(mPlaneAnimTime);

    if (mCurrentShot == 7 || mCurrentShot == 8)
    {
        float shotT = mCinematicTime - mShot7Start;
        float t = Math::Clamp(shotT / (mShot9Start - mShot7Start), 0.0f, 1.0f);
        smoothT.position = Math::Lerp(mDiveStartPos, Math::Vector3{0.0f, 1.5f, 1.0f}, t);
        Quaternion levelRot =
            Quaternion::CreateFromYawPitchRoll(-Math::Constants::Pi * 0.5f, 0.0f, 0.0f);
        Quaternion diveRot = Quaternion::CreateFromYawPitchRoll(
            -Math::Constants::Pi * 0.5f, Math::Constants::Pi * 0.3f, 0.0f);
        smoothT.rotation.x = levelRot.x + (diveRot.x - levelRot.x) * t;
        smoothT.rotation.y = levelRot.y + (diveRot.y - levelRot.y) * t;
        smoothT.rotation.z = levelRot.z + (diveRot.z - levelRot.z) * t;
        smoothT.rotation.w = levelRot.w + (diveRot.w - levelRot.w) * t;
    }

    Transform planeT = smoothT;
    if (mPlaneShaking)
    {
        mShakeTime += deltaTime;
        planeT.position.y += sinf(mShakeTime * 30.0f) * 0.15f;
        planeT.position.x += sinf(mShakeTime * 25.0f) * 0.08f;
    }
    mPlane.transform = planeT;

    if (!mPaused)
    {
        mCinematicTime += deltaTime;
        if (mCinematicTime >= mCinematicEnd)
        {
            mCinematicTime = mCinematicEnd;
            mCinematicDone = true;
        }
    }

    int prevShot = mCurrentShot;
    if (mCinematicTime >= mShot14Start)
        mCurrentShot = 14;
    else if (mCinematicTime >= mShot13Start)
        mCurrentShot = 13;
    else if (mCinematicTime >= mShot12Start)
        mCurrentShot = 12;
    else if (mCinematicTime >= mShot11Start)
        mCurrentShot = 11;
    else if (mCinematicTime >= mShot10Start)
        mCurrentShot = 10;
    else if (mCinematicTime >= mShot9Start)
        mCurrentShot = 9;
    else if (mCinematicTime >= mShot8Start)
        mCurrentShot = 8;
    else if (mCinematicTime >= mShot7Start)
        mCurrentShot = 7;
    else if (mCinematicTime >= mShot6Start)
        mCurrentShot = 6;
    else if (mCinematicTime >= mShot5Start)
        mCurrentShot = 5;
    else if (mCinematicTime >= mShot4Start)
        mCurrentShot = 4;
    else if (mCinematicTime >= mShot3Start)
        mCurrentShot = 3;
    else if (mCinematicTime >= mShot2Start)
        mCurrentShot = 2;
    else
        mCurrentShot = 1;

    if (mCurrentShot != prevShot)
        OnShotEnter(mCurrentShot, smoothT);

    if (!mFreeCam)
    {
        switch (mCurrentShot)
        {
        case 1:
        {
            float st = Math::Clamp(
                (mCinematicTime - mShot1Start) / (mShot2Start - mShot1Start), 0.0f, 1.0f);
            float ease = st * st;
            float camZ = -4.0f + (-2.8f - (-4.0f)) * ease;
            mCamera.SetPosition({0.0f, 1.5f, camZ});
            mCamera.SetLookAt({0.0f, 1.4f, 0.0f});
            break;
        }
        case 2:
            mCamera.SetPosition(smoothT.position + Math::Vector3{0.0f, 1.0f, -12.0f});
            mCamera.SetLookAt(smoothT.position);
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
        case 10:
            mCamera.SetPosition(smoothT.position + Math::Vector3{2.0f, 1.0f, -4.0f});
            mCamera.SetLookAt(smoothT.position);
            mStanley.transform.position = smoothT.position + Math::Vector3{0.0f, 0.5f, 0.0f};
            break;
        case 11:
        {
            float shotT = mCinematicTime - mShot11Start;
            float t = Math::Clamp(shotT / (mShot12Start - mShot11Start), 0.0f, 1.0f);
            Math::Vector3 towerTop = mTower.transform.position + Math::Vector3{0.0f, 21.0f, 0.0f};
            Math::Vector3 planeStart = {-20.0f, towerTop.y, towerTop.z};
            Math::Vector3 planeEnd = towerTop + Math::Vector3{-15.0f, 0.0f, 0.0f};
            Math::Vector3 planePos = Math::Lerp(planeStart, planeEnd, t);
            mPlane.transform.position = planePos;
            mPlane.transform.rotation =
                Quaternion::CreateFromYawPitchRoll(-Math::Constants::Pi * 0.5f, 0.0f, 0.0f);
            mPlane.transform.scale = {0.3f, 0.3f, 0.3f};
            mStanley.transform.position = planePos + Math::Vector3{0.0f, 0.5f, 0.0f};
            mCamera.SetPosition(planePos + Math::Vector3{-6.0f, 2.0f, -8.0f});
            mCamera.SetLookAt(Math::Lerp(planePos, towerTop, 0.5f));
            break;
        }
        case 12:
        {
            float shotT = mCinematicTime - mShot12Start;
            float t = Math::Clamp(shotT / (mShot13Start - mShot12Start), 0.0f, 1.0f);
            Math::Vector3 towerCenter =
                mTower.transform.position + Math::Vector3{0.0f, 21.0f, 0.0f};
            float radius = 30.0f;
            float angle = Math::Constants::Pi + (0.0f - Math::Constants::Pi) * t;
            float arcY = towerCenter.y + sinf(t * Math::Constants::Pi) * 10.0f;
            Math::Vector3 planePos = {
                towerCenter.x + cosf(angle) * radius, arcY, towerCenter.z + sinf(angle) * radius};
            mPlane.transform.position = planePos;
            float yaw = angle - Math::Constants::Pi * 0.5f;
            mPlane.transform.rotation = Quaternion::CreateFromYawPitchRoll(yaw, 0.0f, 0.0f);
            mPlane.transform.scale = {0.3f, 0.3f, 0.3f};
            mStanley.transform.position = planePos + Math::Vector3{0.0f, 0.5f, 0.0f};
            Math::Vector3 camOff = {cosf(angle - 0.5f) * 6.0f, 3.0f, sinf(angle - 0.5f) * 6.0f};
            mCamera.SetPosition(planePos + camOff);
            mCamera.SetLookAt(towerCenter);
            break;
        }
        case 13:
        {
            float shotT = mCinematicTime - mShot13Start;
            float t = Math::Clamp(shotT / (mCinematicEnd - mShot13Start), 0.0f, 1.0f);
            Math::Vector3 towerTop = mTower.transform.position + Math::Vector3{0.0f, 21.0f, 0.0f};
            Math::Vector3 shot12End = towerTop + Math::Vector3{30.0f, 0.0f, 0.0f};
            Math::Vector3 planeEnd = shot12End + Math::Vector3{30.0f, 5.0f, 0.0f};
            Math::Vector3 planePos = Math::Lerp(shot12End, planeEnd, t);
            mPlane.transform.position = planePos;
            float spin = shotT * 15.0f;
            mPlane.transform.rotation = Quaternion::CreateFromYawPitchRoll(
                -Math::Constants::Pi * 0.5f + sinf(spin * 1.1f) * 2.5f,
                sinf(spin * 1.7f) * 3.0f,
                spin);
            mPlane.transform.scale = {0.3f, 0.3f, 0.3f};
            mStanley.transform.position = planePos + Math::Vector3{0.0f, 0.5f, 0.0f};
            mCamera.SetPosition(planePos + Math::Vector3{-4.0f, 3.0f, -6.0f});
            mCamera.SetLookAt(planePos);
            break;
        }
        case 14:
        {
            float shotT = mCinematicTime - mShot14Start;
            float t = Math::Clamp(shotT / (mCinematicEnd - mShot14Start), 0.0f, 1.0f);
            Math::Vector3 giantPos = mGiantStanley.transform.position;
            Math::Vector3 giantCenter = giantPos + Math::Vector3{0.0f, 40.0f, 0.0f};
            Math::Vector3 towerTop2 = mTower.transform.position + Math::Vector3{0.0f, 21.0f, 0.0f};
            Math::Vector3 shot13End2 = towerTop2 + Math::Vector3{60.0f, 5.0f, 0.0f};
            Math::Vector3 planeStart = shot13End2;
            Math::Vector3 planeEnd = giantCenter;
            Math::Vector3 planePos = Math::Lerp(planeStart, planeEnd, t);
            mPlane.transform.position = planePos;
            mPlane.transform.rotation =
                Quaternion::CreateFromYawPitchRoll(-Math::Constants::Pi * 0.5f, 0.0f, 0.0f);
            mPlane.transform.scale = {0.3f, 0.3f, 0.3f};
            mStanley.transform.position = planePos + Math::Vector3{0.0f, 0.5f, 0.0f};
            Math::Vector3 camPos = planePos + Math::Vector3{0.0f, 2.0f, -10.0f};
            mCamera.SetPosition(camPos);
            mCamera.SetLookAt(Math::Lerp(planePos, giantCenter, 0.7f));
            break;
        }
        }
    }
    else
    {
        UpdateCamera(deltaTime);
    }

    mStandardEffect.SetCamera(mCamera);
    mParticleSystemEffect.SetCamera(mCamera);

    if (mExplosion.IsActive())
    {
        mExplosion.SetPositon(mPlane.transform.position);
        mExplosion.Update(deltaTime);
    }

    if (mCurrentShot == 13)
    {
        mExplosion.SetPositon(mPlane.transform.position);
        mExplosion.SpawnParticles();
    }
}

void GameState::OnShotEnter(int shot, const Engine::Graphics::Transform& smoothT)
{
    switch (shot)
    {
    case 2:
        mPlaneAnimTime = 0.0f;
        break;
    case 3:
        mPlaneAnimTime = 0.0f;
        break;
    case 4:
        mPlaneAnimTime = 2.0f;
        mExplosion.SetPositon(mPlaneFlightAnimation.GetTransform(mPlaneAnimTime).position);
        mExplosion.SpawnParticles();
        break;
    case 5:
        mPlaneShaking = true;
        break;
    case 6:
        mPlaneShaking = false;
        mShakeTime = 0.0f;
        break;
    case 7:
        mPlaneAnimTime = 0.0f;
        mDiveStartPos = mPlaneFlightAnimation.GetTransform(0.0f).position;
        break;
    case 8:
        break;
    case 9:
        mPlaneShaking = false;
        mShakeTime = 0.0f;
        mStanley.transform.position = smoothT.position + Math::Vector3{0.0f, 0.5f, 0.0f};
        mStanleyAnimator.PlayAnimation(0, true);
        break;
    case 10:
        mPlaneShaking = true;
        mShakeTime = 0.0f;
        mExplosion.SetPositon(smoothT.position);
        mExplosion.SpawnParticles();
        break;
    case 11:
        if (!mTowerLoaded)
        {
            Quaternion towerRot =
                Quaternion::CreateFromYawPitchRoll(0.0f, Math::Constants::Pi * 0.5f, 0.0f);
            Math::Vector3 towerScale = {0.1f, 0.1f, 0.1f};

            mTower.Initialize("Tower/tower.model");
            mTower.transform.position = {50.0f, 0.0f, 30.0f};
            mTower.transform.rotation = towerRot;
            mTower.transform.scale = towerScale;

            mTower2.Initialize("Tower/tower.model");
            mTower2.transform.position = {50.0f, 0.0f, 50.0f};
            mTower2.transform.rotation = towerRot;
            mTower2.transform.scale = towerScale;

            mTower3.Initialize("Tower/tower.model");
            mTower3.transform.position = {70.0f, 0.0f, 40.0f};
            mTower3.transform.rotation = towerRot;
            mTower3.transform.scale = towerScale;

            mTowerLoaded = true;
        }
        mPlaneShaking = true;
        break;
    case 14:
        if (!mGiantLoaded)
        {
            mGiantStanley.Initialize("stanley/stanley.model");
            Math::Vector3 towerTop = mTower.transform.position + Math::Vector3{0.0f, 21.0f, 0.0f};
            Math::Vector3 shot13End = towerTop + Math::Vector3{60.0f, 5.0f, 0.0f};
            mGiantStanley.transform.position = shot13End + Math::Vector3{60.0f, -5.0f, 0.0f};
            mGiantStanley.transform.scale = {50.0f, 50.0f, 50.0f};
            mGiantStanley.transform.rotation =
                Quaternion::CreateFromYawPitchRoll(-Math::Constants::Pi * 0.5f, 0.0f, 0.0f);
            mGiantStanley.animator = &mGiantAnimator;
            mGiantAnimator.Initialize(mGiantStanley.modelId);
            mGiantAnimator.PlayAnimation(1, true);
            mGiantLoaded = true;
        }
        mPlaneShaking = false;
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
    if (mTowerLoaded)
    {
        mStandardEffect.Render(mTower);
        mStandardEffect.Render(mTower2);
        mStandardEffect.Render(mTower3);
    }
    if (mGiantLoaded)
        mStandardEffect.Render(mGiantStanley);
    mStandardEffect.End();

    mParticleSystemEffect.Begin();
    mExplosion.Render(mParticleSystemEffect);
    mParticleSystemEffect.End();
}

void GameState::DebugUI()
{
    ImGui::Begin("Cinematic Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Shot: %d / 14   Time: %.1f / %.1f", mCurrentShot, mCinematicTime, mCinematicEnd);
    ImGui::Text(
        "Shaking: %s   Done: %s", mPlaneShaking ? "YES" : "NO", mCinematicDone ? "YES" : "NO");

    ImGui::Separator();

    if (ImGui::Button(mPaused ? "Play" : "Pause"))
        mPaused = !mPaused;
    ImGui::SameLine();
    if (ImGui::Button("Reset"))
        ResetCinematic();
    ImGui::SameLine();
    ImGui::Checkbox("Free Cam", &mFreeCam);

    ImGui::Separator();
    ImGui::Text("Skip to shot:");
    for (int i = 1; i <= 14; ++i)
    {
        if (i > 1)
            ImGui::SameLine();
        char label[4];
        snprintf(label, sizeof(label), "%d", i);
        if (ImGui::Button(label))
        {
            float targets[] = {mShot1Start,
                               mShot2Start,
                               mShot3Start,
                               mShot4Start,
                               mShot5Start,
                               mShot6Start,
                               mShot7Start,
                               mShot8Start,
                               mShot9Start,
                               mShot10Start,
                               mShot11Start,
                               mShot12Start,
                               mShot13Start,
                               mShot14Start};
            mCinematicTime = targets[i - 1];
            mCinematicDone = false;
        }
    }

    ImGui::Separator();
    ImGui::Text("Shot Timings:");
    ImGui::DragFloat("Shot 1", &mShot1Start, 0.1f, 0.0f, mShot2Start - 0.5f);
    ImGui::DragFloat("Shot 2", &mShot2Start, 0.1f, mShot1Start + 0.5f, mShot3Start - 0.5f);
    ImGui::DragFloat("Shot 3", &mShot3Start, 0.1f, mShot2Start + 0.5f, mShot4Start - 0.5f);
    ImGui::DragFloat("Shot 4", &mShot4Start, 0.1f, mShot3Start + 0.5f, mShot5Start - 0.5f);
    ImGui::DragFloat("Shot 5", &mShot5Start, 0.1f, mShot4Start + 0.5f, mShot6Start - 0.5f);
    ImGui::DragFloat("Shot 6", &mShot6Start, 0.1f, mShot5Start + 0.5f, mShot7Start - 0.5f);
    ImGui::DragFloat("Shot 7", &mShot7Start, 0.1f, mShot6Start + 0.5f, mShot8Start - 0.5f);
    ImGui::DragFloat("Shot 8", &mShot8Start, 0.1f, mShot7Start + 0.5f, mShot9Start - 0.5f);
    ImGui::DragFloat("Shot 9", &mShot9Start, 0.1f, mShot8Start + 0.5f, mShot10Start - 0.5f);
    ImGui::DragFloat("Shot 10", &mShot10Start, 0.1f, mShot9Start + 0.5f, mShot11Start - 0.5f);
    ImGui::DragFloat("Shot 11", &mShot11Start, 0.1f, mShot10Start + 0.5f, mShot12Start - 0.5f);
    ImGui::DragFloat("Shot 12", &mShot12Start, 0.1f, mShot11Start + 0.5f, mShot13Start - 0.5f);
    ImGui::DragFloat("Shot 13", &mShot13Start, 0.1f, mShot12Start + 0.5f, mShot14Start - 0.5f);
    ImGui::DragFloat("Shot 14", &mShot14Start, 0.1f, mShot13Start + 0.5f, mCinematicEnd - 0.5f);
    ImGui::DragFloat("End", &mCinematicEnd, 0.1f, mShot14Start + 0.5f, 60.0f);

    ImGui::Separator();
    ImGui::Text("Camera Pos: %.1f %.1f %.1f",
                mCamera.GetPosition().x,
                mCamera.GetPosition().y,
                mCamera.GetPosition().z);

    ImGui::Separator();
    ImGui::Text("Lighting:");
    ImGui::DragFloat3("Direction", &mDirectionalLight.direction.x, 0.01f, -1.0f, 1.0f);
    ImGui::ColorEdit4("Ambient##L", &mDirectionalLight.ambient.x);
    ImGui::ColorEdit4("Diffuse##L", &mDirectionalLight.diffuse.x);
    mStandardEffect.SetDirectionalLight(mDirectionalLight);

    ImGui::Separator();
    ImGui::Text("Stanley Material:");
    for (auto& ro : mStanley.renderObjects)
    {
        ImGui::ColorEdit4("Ambient##M", &ro.material.ambient.x);
        ImGui::ColorEdit4("Diffuse##M", &ro.material.diffuse.x);
        ImGui::ColorEdit4("Specular##M", &ro.material.specular.x);
        ImGui::DragFloat("Shininess", &ro.material.shininess, 1.0f, 1.0f, 200.0f);
        break;
    }

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
