#pragma once

#include <Engine/inc/Engine.h>

using namespace Engine;
using namespace Engine::Graphics;

struct Transform {
    Math::Vector3 position = Math::Vector3::Zero;
    Math::Quaternion rotation = Math::Quaternion::Identity;
    Math::Vector3 scale = Math::Vector3::One;

    Math::Matrix4 Apply() const {
        return Math::Matrix4::Translation(position) *
               Math::Matrix4::MatrixRotationQuaternion(rotation) * Math::Matrix4::Scaling(scale);
    }
};

struct RenderObject {
    Math::Matrix4 matWorld = Math::Matrix4::Identity;
    MeshBuffer mesh;
    TextureId textureId = 0;
};

struct PlanetData {
    RenderObject object;
    float orbitRadius;
    float orbitSpeed;
    float rotationSpeed;
    float orbitAngle = 0.0f;
    float rotationAngle = 0.0f;
    float radius = 1.0f;
};

class GameState : public AppState {
  public:
    void Initialize() override;
    void Terminate() override;
    void Update(float deltaTime) override;
    void Render() override;
    void DebugUI() override;

  private:
    void UpdateCamera(float deltaTime);
    void UpdateCelestialBody(PlanetData &body, float deltaTime);
    void DrawOrbit(const PlanetData &body);
    void RenderMesh(const RenderObject &object, const Camera &camera);

    // Core components
    Camera mMainCamera;
    Camera mPlanetCamera;
    RenderTarget mPlanetRenderTarget;

    // GPU Communication
    ConstantBuffer mTransformBuffer;
    VertexShader mVertexShader;
    PixelShader mPixelShader;
    Sampler mSampler;

    // Render Objects
    RenderObject mSkySphere;
    RenderObject mSun;
    std::vector<PlanetData> mPlanets;
    std::vector<std::unique_ptr<RenderObject>> mMoons;

    // UI state
    int mSelectedPlanetIndex;
    bool mShowOrbits;
    float mGlobalSpeedMultiplier;
    bool mShowPlanetView;
};
