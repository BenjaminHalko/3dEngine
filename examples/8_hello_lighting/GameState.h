#pragma once

#include <Engine/inc/Engine.h>

class GameState : public AppState {
  public:
    void Initialize() override;

    void Terminate() override;

    void Update(float deltaTime) override;

    void Render() override;

    void DebugUI() override;

  private:
    void UpdateCamera(float deltaTime);

    Graphics::Camera mCamera;
    Graphics::RenderObject mRenderObject;
};
