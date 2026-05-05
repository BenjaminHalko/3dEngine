#include "GameState.h"

using namespace Engine;
using namespace Engine::Graphics;
using namespace Engine::Input;

void GameState::Initialize()
{
    mGameWorld.AddService<CameraService>();
    mGameWorld.AddService<RenderService>();
    mGameWorld.Initialize();

    GameObject* cameraGO = mGameWorld.CreateGameObject("Camera", L"Assets/Templates/Objects/fps_camera.json");
    cameraGO->Initialize();

    GameObject* transformGO = mGameWorld.CreateGameObject("Transform", L"Assets/Templates/Objects/transform_obj.json");
    transformGO->Initialize();

    GameObject* playerGO = mGameWorld.CreateGameObject("Player", L"Assets/Templates/Objects/transform_obj.json");
    TransformComponent* playerTransform = playerGO->GetComponent<TransformComponent>();
    playerTransform->position.x = 0.0f;
    playerGO->Initialize();

    GameObject* modelGO = mGameWorld.CreateGameObject("SphereObj", L"Assets/Templates/Objects/mesh_obj.json");
    TransformComponent* sphereTransform = modelGO->GetComponent<TransformComponent>();
    sphereTransform->position.x = 3.0f;
    modelGO->Initialize();
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
}
