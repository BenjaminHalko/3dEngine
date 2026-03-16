#include "GameState.h"

using namespace Engine;
using namespace Engine::Graphics;
using namespace Engine::Input;
using namespace Engine::Physics;
using namespace Engine::Audio;
using namespace Engine::Core;

void GameState::Initialize()
{
}

void GameState::Terminate()
{
}

void GameState::Render()
{
}

void GameState::Update(float deltaTime)
{
    UpdateCamera(deltaTime);
}

void GameState::DebugUI()
{
}

void GameState::UpdateCamera(float deltaTime)
{
}
