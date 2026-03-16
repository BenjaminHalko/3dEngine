#include "GameState.h"

using namespace Engine;
using namespace Engine::Graphics;
using namespace Engine::Input;
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

    // Plane model
    mPlane.Initialize("Plane/plane.model");
    mPlane.transform.scale = {0.5f, 0.5f, 0.5f};

    // Flight path animation — flies from left to right overhead
    const float flightDuration = 6.0f;
    mPlaneAnimTime = 0.0f;
    mPlaneFlightAnimation = AnimationBuilder()
        .AddPositionKey({-20.0f, 8.0f,  5.0f}, 0.0f)
        .AddPositionKey({  0.0f, 8.0f,  0.0f}, flightDuration * 0.5f)
        .AddPositionKey({ 20.0f, 8.0f, -5.0f}, flightDuration)
        // Plane faces direction of travel (rotated ~14 deg around Y)
        .AddRotationKey(Quaternion::CreateFromAxisAngle(Vector3::YAxis, -0.24f), 0.0f)
        .AddRotationKey(Quaternion::CreateFromAxisAngle(Vector3::YAxis, -0.24f), flightDuration)
        .AddScaleKey({0.5f, 0.5f, 0.5f}, 0.0f)
        .AddScaleKey({0.5f, 0.5f, 0.5f}, flightDuration)
        .Build();

    // StandardEffect
    std::filesystem::path shaderFile = "Assets/Shaders/Standard.hlsl";
    mStandardEffect.Initialize(shaderFile);
    mStandardEffect.SetCamera(mCamera);
    mStandardEffect.SetDirectionalLight(mDirectionalLight);
}

void GameState::Terminate()
{
    mPlane.Terminate();
    mGround.Terminate();
    mStanley.Terminate();
    mStandardEffect.Terminate();
}

void GameState::Update(float deltaTime)
{
    UpdateCamera(deltaTime);
    mStanleyAnimator.Update(deltaTime);

    // Advance plane animation time (loop)
    mPlaneAnimTime += deltaTime;
    if (mPlaneAnimTime > mPlaneFlightAnimation.GetDuration())
        mPlaneAnimTime -= mPlaneFlightAnimation.GetDuration();

    // Apply flight transform to plane
    mPlane.transform = mPlaneFlightAnimation.GetTransform(mPlaneAnimTime);
}

void GameState::Render()
{
    mStandardEffect.Begin();
    mStandardEffect.Render(mStanley);
    mStandardEffect.Render(mGround);
    mStandardEffect.Render(mPlane);
    mStandardEffect.End();
}

void GameState::DebugUI()
{
    ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Stage 2: Plane Flight Path");
    ImGui::Text("Plane anim time: %.2f / %.2f", mPlaneAnimTime, mPlaneFlightAnimation.GetDuration());
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
