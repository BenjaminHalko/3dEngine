#include "GameState.h"

using namespace Engine;
using namespace Engine::Graphics;
using namespace Engine::Input;


void GameState::Initialize()
{
    mCamera.SetPosition({ 0.0f, 1.5f, -2.0f });
    mCamera.SetLookAt({ 0.0f, 1.0f, 0.0f });

    mDirectionalLight.direction = Math::Normalize({ 1.0f, -1.0f, 1.0f });
    mDirectionalLight.ambient = { 0.4f, 0.4f, 0.4f, 1.0f };
    mDirectionalLight.diffuse = { 0.8f, 0.8f, 0.8f, 1.0f };
    mDirectionalLight.specular = { 0.9f, 0.9f, 0.9f, 1.0f };

    Mesh groundMesh = MeshBuilder::CreatePlane(25, 25, 1.0f);
    mGround.meshBuffer.Initialize(groundMesh);
    mGround.diffuseMapId = TextureManager::Get()->LoadTexture("misc/concrete.jpg");
    mGround.transform.position = { 0.0f, 0.0f, 0.0f };

    mCharacter.Initialize("Character_01/Character_01.model"); // Lil Timmy
    mCharacter.transform.position = { 0.0f, 0.0f, 0.0f };

    parasite.Initialize("parasite/parasite.model"); // Parasite
    parasite.transform.position = { -0.5f, 0.0f, 0.9f };

    zombie.Initialize("zombie/zombie.model"); // Zombie
    zombie.transform.position = { 0.5f, 0.0f, 0.6f };

    MeshPX screenQuadMesh = MeshBuilder::CreateScreenQuadPX();
    mScreenQuad.meshBuffer.Initialize(screenQuadMesh);

    std::filesystem::path shaderFile = L"Assets/Shaders/Standard.fx";
    mStandardEffect.Initialize(shaderFile);
    mStandardEffect.SetCamera(mCamera);
    mStandardEffect.SetDirectionalLight(mDirectionalLight);

    shaderFile = L"Assets/Shaders/PostProcessing.fx";
    mPostProcessingEffect.Initialize(shaderFile);
    mPostProcessingEffect.SetTexture(&mRenderTarget);
    mPostProcessingEffect.SetTexture(&mCombineTexture, 1);

    GraphicsSystem* gs = GraphicsSystem::Get();
    const uint32_t screenWidth = gs->GetBackBufferWidth();
    const uint32_t screenHeight = gs->GetBackBufferHeight();
    mRenderTarget.Initialize(screenWidth, screenHeight, RenderTarget::Format::RGBA_U8);

    mCombineTexture.Initialize(L"Assets/Textures/misc/tree.jpg");
}

void GameState::Terminate()
{
    mCombineTexture.Terminate();
    mRenderTarget.Terminate();
    mScreenQuad.Terminate();
    mCharacter.Terminate();
    parasite.Terminate();
    zombie.Terminate();
    mGround.Terminate();
    mPostProcessingEffect.Terminate();
    mStandardEffect.Terminate();
}

void GameState::Update(float deltaTime)
{
    UpdateCamera(deltaTime);
}

void GameState::Render()
{
//----------------------------------------------------------
    //SimpleDraw::AddGroundPlane(20.0f, Colors::White);
    //SimpleDraw::Render(mCamera);

    mRenderTarget.BeginRender();
        mStandardEffect.Begin();

        mStandardEffect.Render(mGround);
        mStandardEffect.Render(mCharacter);
        mStandardEffect.Render(parasite);
        mStandardEffect.Render(zombie);

        mStandardEffect.End();
    mRenderTarget.EndRender();

//----------------------------------------------------------
    mPostProcessingEffect.Begin();

        mPostProcessingEffect.Render(mScreenQuad);

    mPostProcessingEffect.End();
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

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (uint32_t i = 0; i < mCharacter.renderObjects.size(); ++i)
        {
            Material& material = mCharacter.renderObjects[i].material;
            std::string renderObjectId = "RenderObject " + std::to_string(i);
            ImGui::PushID(renderObjectId.c_str());
            if (ImGui::CollapsingHeader(renderObjectId.c_str()))
            {
                ImGui::LabelText("label", "Material:");
                ImGui::ColorEdit4("Emissive#Material", &material.emissive.r);
                ImGui::ColorEdit4("Ambient#Material", &material.ambient.r);
                ImGui::ColorEdit4("Diffuse#Material", &material.diffuse.r);
                ImGui::ColorEdit4("Specular#Material", &material.specular.r);
                ImGui::DragFloat("Shininess#Material", &material.shininess, 0.1f, 0.1f, 10000.0f);
            }
            ImGui::PopID();
        }
    }

    ImGui::Separator();
    ImGui::Text("Render Target");
    ImGui::Image(mRenderTarget.GetRawData(),
        { 128, 128 },
        { 0, 0 },
        { 1, 1 },
        { 1,1,1,1 },
        { 1,1,1,1 });
    mStandardEffect.DebugUI();
    mPostProcessingEffect.DebugUI();
    ImGui::End();
}

void GameState::UpdateCamera(float deltaTime)
{
    // Camera Controls:
    InputSystem* input = InputSystem::Get();
    const float moveSpeed = input->IsKeyDown(KeyCode::LSHIFT) ? 10.0f : 4.0f;
    const float turnSpeed = 0.5f;

    if (input->IsKeyDown(KeyCode::W)) { mCamera.Walk(moveSpeed * deltaTime); }

    else if (input->IsKeyDown(KeyCode::S)) { mCamera.Walk(-moveSpeed * deltaTime); }

    else if (input->IsKeyDown(KeyCode::D)) { mCamera.Strafe(moveSpeed * deltaTime); }

    else if (input->IsKeyDown(KeyCode::A)) { mCamera.Strafe(-moveSpeed * deltaTime); }

    else if (input->IsKeyDown(KeyCode::E)) { mCamera.Rise(moveSpeed * deltaTime); }

    else if (input->IsKeyDown(KeyCode::Q)) { mCamera.Rise(-moveSpeed * deltaTime); }

    if (input->IsMouseDown(MouseButton::RBUTTON))
    {
        mCamera.Yaw(input->GetMouseMoveX() * turnSpeed * deltaTime);
        mCamera.Pitch(input->GetMouseMoveY() * turnSpeed * deltaTime);
    }
}


