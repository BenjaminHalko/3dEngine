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
    struct Object {
        Engine::Math::Matrix4 worldMat = Engine::Math::Matrix4::Identity;
        Engine::Graphics::MeshBuffer meshBuffer;
        Engine::Graphics::TextureId textureId = 0;
    };

    void UpdateCamera(float deltaTime);

    void RenderObject(const Object &object, const Engine::Graphics::Camera &camera);

    Engine::Graphics::Camera mCamera;
    Engine::Graphics::Camera mRenderTargetCamera;

    // GPU Communication
    Engine::Graphics::ConstantBuffer mTransformBuffer;
    Engine::Graphics::VertexShader mVertexShader;
    Engine::Graphics::PixelShader mPixelShader;
    Engine::Graphics::Sampler mSampler;

    // Render Object
    Object mSpace;
    Object mEarth;
    Object mSun;

    // Render Target
    Engine::Graphics::RenderTarget mRenderTarget;
};
