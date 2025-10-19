#pragma once

#include <Engine/Inc/Engine.h>

class GameState : public Engine::AppState {
  public:
    void Initialize() override;

    void Terminate() override;

    void Update(float deltaTime) override;

    void Render() override;

    void DebugUI() override;

  private:
    void UpdateCamera(float deltaTime);

    Engine::Graphics::Camera mCamera;
};