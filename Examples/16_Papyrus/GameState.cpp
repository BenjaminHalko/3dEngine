#include "GameState.h"

using namespace Engine;
using namespace Engine::Graphics;
using namespace Engine::Input;

enum class CurrentModel
{
    Timmy,
    Parasite,
    Zombie
};

const char* gObjectNames[] = {"Timmy", "Parasite", "Zombie"};

CurrentModel gCurrentModel = CurrentModel::Timmy;

void GameState::Initialize()
{
    mCamera.SetPosition({0.0f, 1.0f, -3.0f});
    mCamera.SetLookAt({0.0f, 0.0f, 0.0f});

    mDirectionalLight.direction = Math::Normalize({1.0f, -1.0f, 1.0f});
    mDirectionalLight.ambient = {0.4f, 0.4f, 0.4f, 1.0f};
    mDirectionalLight.diffuse = {0.8f, 0.8f, 0.8f, 1.0f};
    mDirectionalLight.specular = {0.9f, 0.9f, 0.9f, 1.0f};

    mCharacter.Initialize("Character_01/Character_01.model");
    mCharacter.transform.position = {0.0f, 0.0f, 0.0f};

    mParasite.Initialize("parasite/parasite.model");
    mParasite.transform.position = {0.0f, 0.0f, 0.0f};

    mZombie.Initialize("zombie/zombie.model");
    mZombie.transform.position = {0.0f, 0.0f, 0.0f};

    std::filesystem::path shaderFile = "Assets/Shaders/Standard.hlsl";
    mStandardEffect.Initialize(shaderFile);
    mStandardEffect.SetCamera(mCamera);
    mStandardEffect.SetDirectionalLight(mDirectionalLight);
}

void GameState::Terminate()
{
    mCharacter.Terminate();
    mParasite.Terminate();
    mZombie.Terminate();
    mStandardEffect.Terminate();
}

void GameState::Update(float deltaTime)
{
    UpdateCamera(deltaTime);
}

void GameState::Render()
{
    SimpleDraw::AddGroundPlane(20.0f, mDrawSkeleton ? Colors::DarkRed : Colors::White);
    SimpleDraw::Render(mCamera);

    if (mDrawSkeleton)
    {
        AnimationUtil::BoneTransforms boneTransforms;

        switch (gCurrentModel)
        {
        case CurrentModel::Timmy:
            AnimationUtil::ComputeBoneTransforms(mCharacter.modelId, boneTransforms);
            AnimationUtil::DrawSkeleton(mCharacter.modelId, boneTransforms);
            break;
        case CurrentModel::Parasite:
            AnimationUtil::ComputeBoneTransforms(mParasite.modelId, boneTransforms);
            AnimationUtil::DrawSkeleton(mParasite.modelId, boneTransforms);
            break;
        case CurrentModel::Zombie:
            AnimationUtil::ComputeBoneTransforms(mZombie.modelId, boneTransforms);
            AnimationUtil::DrawSkeleton(mZombie.modelId, boneTransforms);
            break;
        }
    }
    else
    {
        mStandardEffect.Begin();

        switch (gCurrentModel)
        {
        case CurrentModel::Timmy:
            mStandardEffect.Render(mCharacter);
            break;
        case CurrentModel::Parasite:
            mStandardEffect.Render(mParasite);
            break;
        case CurrentModel::Zombie:
            mStandardEffect.Render(mZombie);
            break;
        }

        mStandardEffect.End();
    }
}

void GameState::DebugUI()
{
    ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::CollapsingHeader("Model Selection", ImGuiTreeNodeFlags_DefaultOpen))
    {
        int currentModelIndex = static_cast<int>(gCurrentModel);
        if (ImGui::Combo("Current Model", &currentModelIndex, gObjectNames, std::size(gObjectNames)))
        {
            gCurrentModel = static_cast<CurrentModel>(currentModelIndex);
        }

        ImGui::Checkbox("Draw Skeleton", &mDrawSkeleton);
    }

    ImGui::Separator();

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
        std::vector<RenderObject>* currentRenderObjects = nullptr;

        switch (gCurrentModel)
        {
        case CurrentModel::Timmy:
            currentRenderObjects = &mCharacter.renderObjects;
            break;
        case CurrentModel::Parasite:
            currentRenderObjects = &mParasite.renderObjects;
            break;
        case CurrentModel::Zombie:
            currentRenderObjects = &mZombie.renderObjects;
            break;
        }

        if (currentRenderObjects)
        {
            for (uint32_t i = 0; i < currentRenderObjects->size(); ++i)
            {
                Material& material = (*currentRenderObjects)[i].material;
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
    }

    ImGui::Separator();

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
