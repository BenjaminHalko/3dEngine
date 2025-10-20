#include "GameState.h"

using namespace Engine;
using namespace Engine::Graphics;
using namespace Engine::Input;

void GameState::Initialize()
{
    mCamera.SetPosition({0.0f, 1.0f, -3.0f});
    mCamera.SetLookAt({0.0f, 0.0f, 0.0f});

    mDirectionalLight.direction = Math::Normalize({1.0f, -1.0f, 1.0f});
    mDirectionalLight.ambient = {0.4f, 0.4f, 0.4f, 1.0f};
    mDirectionalLight.diffuse = {0.8f, 0.8f, 0.8f, 1.0f};
    mDirectionalLight.specular = {0.9f, 0.9f, 0.9f, 1.0f};

    Mesh earth = MeshBuilder::CreateSphere(100, 100, 1.0f);
    mRenderObject.meshBuffer.Initialize(earth);

    TextureManager* tm = TextureManager::Get();
    mRenderObject.diffuseMapId = tm->LoadTexture(L"earth.jpg");
    mRenderObject.specMapId = tm->LoadTexture(L"earth_spec.jpg");
    mRenderObject.normalMapId = tm->LoadTexture(L"earth_normal.jpg");
    mRenderObject.bumpMapId = tm->LoadTexture(L"earth_bump.jpg");

    std::filesystem::path shaderFile = L"Assets/Shaders/Standard.fx";
    mStandardEffect.Initialize(shaderFile);
    mStandardEffect.SetCamera(mCamera);
    mStandardEffect.SetDirectionalLight(mDirectionalLight);
}

void GameState::Terminate()
{
    mRenderObject.Terminate();
    mStandardEffect.Terminate();
}

void GameState::Update(float deltaTime)
{
    UpdateCamera(deltaTime);
}

void GameState::Render()
{
    SimpleDraw::AddGroundPlane(20.0f, Colors::Wheat);
    SimpleDraw::Render(mCamera);

    mStandardEffect.Begin();
    mStandardEffect.Render(mRenderObject);
    mStandardEffect.End();
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

        ImGui::ColorEdit4("Ambient#Light", &mDirectionalLight.ambient.r);
        ImGui::ColorEdit4("Diffuse#Light", &mDirectionalLight.diffuse.r);
        ImGui::ColorEdit4("Specular#Light", &mDirectionalLight.specular.r);
    }

    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::ColorEdit4("Emissive#Material", &mRenderObject.material.emissive.r);
        ImGui::ColorEdit4("Ambient#Material", &mRenderObject.material.ambient.r);
        ImGui::ColorEdit4("Diffuse#Material", &mRenderObject.material.diffuse.r);
        ImGui::ColorEdit4("Specular#Material", &mRenderObject.material.specular.r);
        ImGui::DragFloat(
            "Shininess#Material", &mRenderObject.material.shininess, 0.01f, 0.0f, 10000.0f);
    }

    mStandardEffect.DebugUI();
    ImGui::End();
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
}
