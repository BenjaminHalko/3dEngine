#include "GameState.h"

using namespace Engine;
using namespace Engine::Graphics;
using namespace Engine::Input;

void GameState::Initialize()
{
    mGameWorld.Initialize();

    GameObject* cameraGO = mGameWorld.CreateGameObject("Camera");
    mCameraComponent = cameraGO->AddComponent<CameraComponent>();
    cameraGO->AddComponent<FPSCameraComponent>();
    cameraGO->Initialize();

    GameObject* playerGO = mGameWorld.CreateGameObject("Player");
    TransformComponent* playerTransform = playerGO->AddComponent<TransformComponent>();
    playerTransform->position.x = 0.0f;
    playerGO->Initialize();

    GameObject* enemy1GO = mGameWorld.CreateGameObject("Enemy1");
    TransformComponent* enemy1Transform = enemy1GO->AddComponent<TransformComponent>();
    enemy1Transform->position.x = 5.0f;
    enemy1Transform->position.z = 5.0f;
    enemy1GO->Initialize();

    GameObject* enemy2GO = mGameWorld.CreateGameObject("Enemy2");
    TransformComponent* enemy2Transform = enemy2GO->AddComponent<TransformComponent>();
    enemy2Transform->position.x = 5.0f;
    enemy2Transform->position.z = -5.0f;
    enemy2GO->Initialize();

    GameObject* bossGO = mGameWorld.CreateGameObject("Boss");
    TransformComponent* bossTransform = bossGO->AddComponent<TransformComponent>();
    bossTransform->position.x = 15.0f;
    bossGO->Initialize();

    GameObject* cloudGO = mGameWorld.CreateGameObject("Cloud");
    TransformComponent* cloudTransform = cloudGO->AddComponent<TransformComponent>();
    cloudTransform->position.x = 7.0f;
    cloudTransform->position.y = 7.0f;
    cloudGO->Initialize();
}

void GameState::Terminate()
{
    mGameWorld.Terminate();
}

void GameState::Update(float deltaTime)
{
    mGameWorld.Update(deltaTime);
}

void GameState::Render()
{
    mGameWorld.Render();
}

void GameState::DebugUI()
{
    ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    mGameWorld.DebugUI();
    ImGui::End();

    SimpleDraw::Render(mCameraComponent->GetCamera());
}
