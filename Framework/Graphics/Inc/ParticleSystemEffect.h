#pragma once

#include "BlendState.h"
#include "Color.h"
#include "ConstantBuffer.h"
#include "MeshBuffer.h"
#include "PixelShader.h"
#include "Sampler.h"
#include "TextureManager.h"
#include "VertexShader.h"

namespace Engine::Graphics
{
class Camera;
struct Transform;

class ParticleSystemEffect final
{
  public:
    void Initialize();
    void Terminate();

    void Begin();
    void End();

    void Render(const Transform& transform, const Color& color);

    void DebugUI();
    void SetCamera(const Camera& camera);
    void SetTextureId(TextureId id);

  private:
    using ParticleBuffer = TypedConstantBuffer<Math::Matrix4>;
    using ColorBuffer = TypedConstantBuffer<Color>;
    ParticleBuffer mParticleBuffer;
    ColorBuffer mColorBuffer;

    VertexShader mVertexShader;
    PixelShader mPixelShader;
    Sampler mSampler;
    BlendState mBlendState;

    MeshBuffer mParticle;

    TextureId mTextureId = 0;
    const Camera* mCamera = nullptr;
};
} // namespace Engine::Graphics
