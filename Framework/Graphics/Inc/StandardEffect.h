#pragma once

#include "Common.h"
#include "ConstantBuffer.h"
#include "PixelShader.h"
#include "VertexShader.h"
#include "DirectionalLight.h"
#include "Material.h"
#include "Sampler.h"

namespace Engine::Graphics
{
class Camera;
class RenderObject;
class RenderGroup;
class Texture;

class StandardEffect final
{
  public:
    void Initialize(const std::filesystem::path& path);
    void Terminate();

    void Begin();
    void End();

    void Render(const RenderObject& renderObject);
    void Render(const RenderGroup& renderGroup);

    void SetCamera(const Camera& camera);

    void SetDirectionalLight(const DirectionalLight& directionalLight);
    // Shadows
    void SetLightCamera(const Camera& camera);
    void SetShadowMap(const Texture& shadowMap);

    void DebugUI();

  private:
    struct TransformData
    {
        Math::Matrix4 wvp;
        Math::Matrix4 world;
        Math::Matrix4 lwvp; // Light World-View-Projection matrix
        Math::Vector3 viewPosition;
        float padding = 0.0f;
    };

    struct SettingsData
    {
        int useDiffuseMap = 1;
        int useSpecMap = 1;
        int useNormalMap = 1;
        int useBumpMap = 1;
        int useShadowMap = 1;
        float bumpIntensity = -0.02f;
        float depthBias = 0.000003f;
        float padding = 0.0f;
    };

    using TransformBuffer = TypedConstantBuffer<TransformData>;
    using LightBuffer = TypedConstantBuffer<DirectionalLight>;
    using MaterialBuffer = TypedConstantBuffer<Material>;
    using SettingsBuffer = TypedConstantBuffer<SettingsData>;

    TransformBuffer mTransformBuffer;
    LightBuffer mLightBuffer;
    MaterialBuffer mMaterialBuffer;
    SettingsBuffer mSettingsBuffer;

    VertexShader mVertexShader;
    PixelShader mPixelShader;
    Sampler mSampler;

    const Camera* mCamera = nullptr;
    const DirectionalLight* mDirectionalLight = nullptr;

    const Camera* mLightCamera = nullptr;
    const Texture* mShadowMap = nullptr;

    SettingsData mSettingsData;
};
} // namespace Engine::Graphics
