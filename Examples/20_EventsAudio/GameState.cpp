#include "GameState.h"
#include "GameEvents.h"

using namespace Engine;
using namespace Engine::Graphics;
using namespace Engine::Input;
using namespace Engine::Physics;
using namespace Engine::Audio;
using namespace Engine::Core;

void GameState::Initialize()
{
    mCamera.SetPosition({2.0f, 2.0f, -2.0f});
    mCamera.SetLookAt({0.0f, 1.2f, 0.0f});

    mDirectionalLight.direction = Math::Normalize({1.0f, -1.0f, 1.0f});
    mDirectionalLight.ambient = {0.4f, 0.4f, 0.4f, 1.0f};
    mDirectionalLight.diffuse = {0.8f, 0.8f, 0.8f, 1.0f};
    mDirectionalLight.specular = {0.9f, 0.9f, 0.9f, 1.0f};

    // Particles
    mParticleSystemEffect.Initialize();
    mParticleSystemEffect.SetCamera(mCamera);

    ParticleSystemInfo info;
    info.textureId = TextureManager::Get()->LoadTexture("Images/bullet_bill.png");
    info.maxParticles = 1000;
    info.particlesPerEmit = {8, 12};
    info.delay = 1.0f;
    info.lifeTime = FLT_MAX;
    info.timeBetweenEmit = {0.1f, 0.2f};
    info.spawnAngle = {-20.0f, 20.0f};
    info.spawnSpeed = {1.0f, 3.0f};
    info.particleLifeTime = {0.5f, 2.0f};
    info.spawnDirection = -Math::Vector3::YAxis;
    info.spawnPosition = Math::Vector3::Zero;
    info.startScale = {Math::Vector3::One, Math::Vector3::One};
    info.endScale = {Math::Vector3::One, Math::Vector3::One};
    info.startColour = {Colors::White, Colors::White};
    info.endColour = {Colors::White, Colors::White};
    mParticleSystem.Initialize(info);

    info.textureId = TextureManager::Get()->LoadTexture("Images/explosion.png");
    info.particlesPerEmit = {10, 250};
    info.spawnSpeed = {7.0f, 25.0f};
    info.delay = 0.0f;
    info.lifeTime = 0.0f;
    info.spawnAngle = {-180.0f, 180.0f};
    info.spawnDirection = Math::Vector3::YAxis;
    info.startColour = {Colors::OrangeRed, Colors::LightYellow};
    info.endColour = {Colors::LightGray, Colors::White};
    mFireworkParticles.Initialize(info);

    // Football
    Mesh football = MeshBuilder::CreateSphere(50, 50, 0.5f);
    mFootball.meshBuffer.Initialize(football);
    mFootball.transform.position.y = 5.0f;
    mBallShape.InitializeSphere(0.5f);
    mBallRigidBody.Initialize(mFootball.transform, mBallShape, 5.0f);

    TextureManager* tm = TextureManager::Get();
    mFootball.diffuseMapId = tm->LoadTexture("misc/Brazuca.jpg");

    // Ground
    Mesh plane = MeshBuilder::CreatePlane(20, 20, 1.0f, true);
    mGroundObject.meshBuffer.Initialize(plane);
    mGroundShape.InitializeHull({10.0f, 1.0f, 10.0f}, {0.0f, -0.5f, 0.0f});
    mGroundRigidBody.Initialize(mGroundObject.transform, mGroundShape, 0.0f);

    mGroundObject.diffuseMapId = tm->LoadTexture("misc/concrete.jpg");

    std::filesystem::path shaderFile = "Assets/Shaders/Standard.hlsl";
    mStandardEffect.Initialize(shaderFile);
    mStandardEffect.SetCamera(mCamera);
    mStandardEffect.SetDirectionalLight(mDirectionalLight);

    // Boxes
    Mesh boxMesh = MeshBuilder::CreateCube(1.0f);
    TextureId boxTextureId = tm->LoadTexture("misc/cardboard.jpg");

    float yOffset = 4.5f;
    float xOffset = 0.0f;
    int rowCount = 1;
    int boxIndex = 0;
    mBoxes.resize(10);
    while (boxIndex < static_cast<int>(mBoxes.size()))
    {
        xOffset = -((static_cast<float>(rowCount) - 1.0f) * 0.5f);
        for (int r = 0; r < rowCount; ++r)
        {
            BoxData& box = mBoxes[boxIndex];
            box.box.meshBuffer.Initialize(boxMesh);
            box.box.diffuseMapId = boxTextureId;
            box.box.transform.position.x = xOffset;
            box.box.transform.position.y = yOffset;
            box.box.transform.position.z = 4.0f;
            box.shape.InitializeBox({0.5f, 0.5f, 0.5f});
            xOffset += 1.0f;
            ++boxIndex;
        }
        yOffset -= 1.0f;
        rowCount += 1;
    }
    for (int i = static_cast<int>(mBoxes.size()) - 1; i >= 0; --i)
    {
        mBoxes[i].rigidBody.Initialize(mBoxes[i].box.transform, mBoxes[i].shape, 1.0f);
    }

    // Cloth
    int rows = 20;
    int cols = 20;
    mClothMesh = MeshBuilder::CreatePlane(rows, cols, 0.5f);
    for (Vertex& v : mClothMesh.vertices)
    {
        v.position.y += 10.0f;
        v.position.z += 10.0f;
    }

    uint32_t lastVertex = static_cast<uint32_t>(mClothMesh.vertices.size()) - 1;
    uint32_t lastVertexOS = lastVertex - cols;
    mClothSoftBody.Initialize(mClothMesh, 1.0f, {lastVertex, lastVertexOS});

    mClothRenderObject.meshBuffer.Initialize(nullptr,
                                             static_cast<uint32_t>(sizeof(Vertex)),
                                             static_cast<uint32_t>(mClothMesh.vertices.size()),
                                             mClothMesh.indices.data(),
                                             static_cast<uint32_t>(mClothMesh.indices.size()));
    mClothRenderObject.diffuseMapId = tm->LoadTexture("misc/cloth.jpg");

    // Events
    EventManager* em = EventManager::Get();
    mSpacePressedListenerId =
        em->AddListener(PressSpaceEvent::StaticGetTypeId(),
                        std::bind(&GameState::OnSpacePressedEvent, this, std::placeholders::_1));

    mEnterPressedListenerId = em->AddListener(PressEnterEvent::StaticGetTypeId(),
                                              [](const Event& e) { LOG("E N T E R  WAZ HERE!"); });

    // Sounds
    SoundEffectManager* sm = SoundEffectManager::Get();
    mLaunchSoundId = sm->Load("megamanx_blast.wav");
    mExplosionSoundId = sm->Load("explosion.wav");

    auto launchFirework = [&]() { SoundEffectManager::Get()->Play(mLaunchSoundId); };

    auto spawnFirework = [&]()
    {
        mFireworkParticles.SpawnParticles();
        SoundEffectManager::Get()->Play(mExplosionSoundId);
    };

    mFireworkAnimation = AnimationBuilder()
                             .AddPositionKey(Math::Vector3::Zero, 0.0f)
                             .AddPositionKey(Math::Vector3::Zero, 2.0f)
                             .AddPositionKey(Math::Vector3::YAxis * 25.0f, 4.0f)
                             .AddEventKey(launchFirework, 2.0f)
                             .AddEventKey(spawnFirework, 4.0f)
                             .Build();
}

void GameState::Terminate()
{
    SoundEffectManager* sm = SoundEffectManager::Get();
    sm->Stop(mLaunchSoundId);
    sm->Stop(mExplosionSoundId);

    EventManager* em = EventManager::Get();
    em->RemoveListener(PressSpaceEvent::StaticGetTypeId(), mSpacePressedListenerId);
    em->RemoveListener(PressEnterEvent::StaticGetTypeId(), mEnterPressedListenerId);

    mFireworkParticles.Terminate();

    mParticleSystem.Terminate();
    mParticleSystemEffect.Terminate();

    mClothRenderObject.Terminate();
    mClothSoftBody.Terminate();

    for (BoxData& box : mBoxes)
    {
        box.rigidBody.Terminate();
        box.shape.Terminate();
        box.box.Terminate();
    }

    mStandardEffect.Terminate();

    mGroundRigidBody.Terminate();
    mGroundShape.Terminate();
    mGroundObject.Terminate();

    mBallRigidBody.Terminate();
    mBallShape.Terminate();
    mFootball.Terminate();
}

void GameState::Update(float deltaTime)
{
    UpdateCamera(deltaTime);

    if (InputSystem::Get()->IsKeyPressed(KeyCode::SPACE))
    {
        Math::Vector3 spawnPos = mCamera.GetPosition() + (mCamera.GetDirection() * 0.5f);
        mBallRigidBody.SetPosition(spawnPos);
        mBallRigidBody.SetVelocity(mCamera.GetDirection() * 50.0f);
    }

    mParticleSystem.Update(deltaTime);
    mFireworkParticles.Update(deltaTime);

    float prevTime = mFireworkAnimationTime;
    mFireworkAnimationTime += deltaTime;
    if (mFireworkAnimation.GetDuration() > 0)
    {
        mFireworkAnimation.PlayEvents(prevTime, mFireworkAnimationTime);
        while (mFireworkAnimationTime >= mFireworkAnimation.GetDuration())
        {
            mFireworkAnimationTime -= mFireworkAnimation.GetDuration();
        }
    }
}

void GameState::Render()
{
    mClothRenderObject.meshBuffer.Update(mClothMesh.vertices.data(),
                                         static_cast<uint32_t>(mClothMesh.vertices.size()));

    SimpleDraw::AddGroundPlane(20.0f, Colors::Wheat);
    SimpleDraw::Render(mCamera);

    mStandardEffect.Begin();
    mStandardEffect.Render(mFootball);
    mStandardEffect.Render(mGroundObject);
    mStandardEffect.Render(mClothRenderObject);
    for (BoxData& box : mBoxes)
    {
        mStandardEffect.Render(box.box);
    }
    mStandardEffect.End();

    Transform particleTransform = mFireworkAnimation.GetTransform(mFireworkAnimationTime);
    mParticleSystem.SetPositon(particleTransform.position);
    mFireworkParticles.SetPositon(particleTransform.position);

    mParticleSystemEffect.Begin();
    mParticleSystem.Render(mParticleSystemEffect);
    mFireworkParticles.Render(mParticleSystemEffect);
    mParticleSystemEffect.End();
}

void GameState::DebugUI()
{
    ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::DragFloat3("Direction#Light", &mDirectionalLight.direction.x, 0.01f))
        {
            mDirectionalLight.direction = Math::Normalize(mDirectionalLight.direction);
        }

        ImGui::ColorEdit4("Ambient#Light", &mDirectionalLight.ambient.x);
        ImGui::ColorEdit4("Diffuse#Light", &mDirectionalLight.diffuse.x);
        ImGui::ColorEdit4("Specular#Light", &mDirectionalLight.specular.x);
    }

    ImGui::Separator();

    Math::Vector3 pos = mFootball.transform.position;
    if (ImGui::DragFloat3("BallPosition", &pos.x))
    {
        mFootball.transform.position = pos;
        mBallRigidBody.SetPosition(mFootball.transform.position);
    }

    mStandardEffect.DebugUI();
    mParticleSystem.DebugUI();
    PhysicsWorld::Get()->DebugUI();
    ImGui::End();
    SimpleDraw::Render(mCamera);
}

void GameState::UpdateCamera(float deltaTime)
{
    InputSystem* input = InputSystem::Get();
    const float moveSpeed = input->IsKeyDown(KeyCode::LSHIFT) ? 10.0f : 4.0f;
    const float turnSpeed = 0.5f;

    if (input->IsKeyDown(KeyCode::W))
    {
        mCamera.Walk(moveSpeed * deltaTime);
    }
    else if (input->IsKeyDown(KeyCode::S))
    {
        mCamera.Walk(-moveSpeed * deltaTime);
    }
    else if (input->IsKeyDown(KeyCode::D))
    {
        mCamera.Strafe(moveSpeed * deltaTime);
    }
    else if (input->IsKeyDown(KeyCode::A))
    {
        mCamera.Strafe(-moveSpeed * deltaTime);
    }
    else if (input->IsKeyDown(KeyCode::E))
    {
        mCamera.Rise(moveSpeed * deltaTime);
    }
    else if (input->IsKeyDown(KeyCode::Q))
    {
        mCamera.Rise(-moveSpeed * deltaTime);
    }

    if (input->IsMouseDown(MouseButton::RBUTTON))
    {
        mCamera.Yaw(input->GetMouseMoveX() * turnSpeed * deltaTime);
        mCamera.Pitch(input->GetMouseMoveY() * turnSpeed * deltaTime);
    }

    if (input->IsKeyPressed(KeyCode::SPACE))
    {
        PressSpaceEvent event;
        EventManager::Broadcast(event);
    }

    if (input->IsKeyPressed(KeyCode::RETURN))
    {
        PressEnterEvent event;
        EventManager::Broadcast(event);
    }
}

void GameState::OnSpacePressedEvent(const Engine::Core::Event& e)
{
    LOG("S P A C E  WAZ HERE!");
    mFireworkParticles.SpawnParticles();
}
