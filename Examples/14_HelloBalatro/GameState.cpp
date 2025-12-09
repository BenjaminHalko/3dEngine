#include "GameState.h"

using namespace Engine;
using namespace Engine::Graphics;

void GameState::Initialize()
{
    MeshPX screenQuadMesh = MeshBuilder::CreateScreenQuadPX();
    mScreenQuad.meshBuffer.Initialize(screenQuadMesh);

    std::filesystem::path shaderFile = L"Assets/Shaders/Balatro.fx";
    mBalatroEffect.Initialize(shaderFile);
}

void GameState::Terminate()
{
    mBalatroEffect.Terminate();
    mScreenQuad.Terminate();
}

void GameState::Update(float deltaTime)
{
    mBalatroEffect.Update(deltaTime);
}

void GameState::Render()
{
    mBalatroEffect.Begin();
    mBalatroEffect.Render(mScreenQuad);
    mBalatroEffect.End();
}

void GameState::DebugUI()
{
    ImGui::Begin("Balatro Background", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    mBalatroEffect.DebugUI();
    ImGui::End();
}
