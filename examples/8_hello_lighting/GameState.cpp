#include "GameState.h"

using namespace Engine;
using namespace Engine::Graphics;
using namespace Engine::Input;

void GameState::Initialize() {
    mCamera.SetPosition({0.0f, 1.0f, -3.0f});
    mCamera.SetLookAt({0.0f, 0.0f, 0.0f});
}

void GameState::Terminate() {

}

void GameState::Update(float deltaTime) { UpdateCamera(deltaTime); }

void GameState::Render() {
    SimpleDraw::AddGroundPlane(20.0f, Colors::White);
    SimpleDraw::Render(mCamera);
}

void GameState::DebugUI() {
    ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::End();
}

void GameState::UpdateCamera(float deltaTime) {
    // Camera Controls:
    InputSystem *input = InputSystem::Get();
    const float moveSpeed = input->IsKeyDown(KeyCode::LSHIFT) ? 10.0f : 4.0f;
    const float turnSpeed = 0.5f;

    if (input->IsKeyDown(KeyCode::W)) {
        mCamera.Walk(moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::S)) {
        mCamera.Walk(-moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::D)) {
        mCamera.Strafe(moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::A)) {
        mCamera.Strafe(-moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::E)) {
        mCamera.Rise(moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::Q)) {
        mCamera.Rise(-moveSpeed * deltaTime);
    }

    if (input->IsMouseDown(MouseButton::RBUTTON)) {
        mCamera.Yaw(input->GetMouseMoveX() * turnSpeed * deltaTime);
        mCamera.Pitch(input->GetMouseMoveY() * turnSpeed * deltaTime);
    }
}
