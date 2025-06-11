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

struct CelestialBody {
    std::string name;
    MeshBuffer mesh;
    Texture texture;
    Transform transform;
    float orbitRadius;
    float orbitSpeed;    // Year
    float rotationSpeed; // Day
    float orbitAngle;
    float rotationAngle;
    bool showOrbit;
    std::unique_ptr<CelestialBody> moon;
};

class GameState : public AppState {
  public:
    void Initialize() override;
    void Terminate() override;
    void Update(float deltaTime) override;
    void Render() override;
    void DebugUI() override;

  private:
    void CreateSkySphere();
    void CreateSun();
    void CreatePlanets();
    CelestialBody CreatePlanet(const std::string &name, float size, float orbitRadius,
                               float orbitSpeed, float rotationSpeed,
                               const std::string &textureFile);
    void UpdateCelestialBody(CelestialBody &body, float deltaTime);
    void DrawOrbit(const CelestialBody &body);
    void RenderPlanetView();
    void RenderObject(const CelestialBody &body, const Camera &camera);

    // Core components
    Camera mMainCamera;
    Camera mPlanetCamera;
    RenderTarget mPlanetRenderTarget;
    MeshBuffer mSkySphere;
    Texture mSkyTexture;
    CelestialBody mSun;
    std::vector<CelestialBody> mPlanets;

    // GPU Communication
    ConstantBuffer mTransformBuffer;
    VertexShader mVertexShader;
    PixelShader mPixelShader;
    Sampler mSampler;

    // UI state
    int mSelectedPlanetIndex;
    bool mShowOrbits;
    float mGlobalSpeedMultiplier;
    bool mShowPlanetView;
};
