#include "Precompiled.h"
#include "ParticleSystemEffect.h"

#include "Camera.h"
#include "MeshBuilder.h"
#include "Transform.h"
#include "VertexTypes.h"

using namespace Engine;
using namespace Engine::Graphics;

void ParticleSystemEffect::Initialize()
{
    std::filesystem::path shaderPath = "Assets/Shaders/Particle.hlsl";
    mVertexShader.Initialize<VertexPX>(shaderPath);
    mPixelShader.Initialize(shaderPath);
    mParticleBuffer.Initialize();
    mColorBuffer.Initialize();
    mSampler.Initialize(Sampler::Filter::Linear, Sampler::AddressMode::Wrap);
    mBlendState.Initialize(BlendState::Mode::AlphaBlend);

    MeshPX spriteQuad = MeshBuilder::CreateSpriteQuadPX(0.5f, 0.5f);
    mParticle.Initialize(spriteQuad);
}

void ParticleSystemEffect::Terminate()
{
    mParticle.Terminate();
    mBlendState.Terminate();
    mSampler.Terminate();
    mColorBuffer.Terminate();
    mParticleBuffer.Terminate();
    mPixelShader.Terminate();
    mVertexShader.Terminate();
}

void ParticleSystemEffect::Begin()
{
    mVertexShader.Bind();
    mPixelShader.Bind();
    mParticleBuffer.BindVS(0);
    mColorBuffer.BindPS(1);
    mSampler.BindPS(0);
    mBlendState.Set();
}

void ParticleSystemEffect::End()
{
    mBlendState.ClearState();
    Texture::UnbindPS(0);
}

void ParticleSystemEffect::Render(const Transform& transform, const Color& color)
{
    ASSERT(mTextureId != 0 && mCamera != nullptr, "ParticleSystemEffect: Missing Texture or Camera!");

    TextureManager::Get()->BindPS(mTextureId, 0);

    // Local position of the particle relative to the camera
    const Math::Vector3 localPos = Math::TransformCoord(transform.position, mCamera->GetViewMatrix());
    const Math::Matrix4 matLocalTrans = Math::Matrix4::Translation(localPos);
    const Math::Matrix4 matScale = Math::Matrix4::Scaling(transform.scale);
    const Math::Matrix4 matProj = mCamera->GetProjectionMatrix();
    const Math::Matrix4 matFinal = Math::Transpose(matScale * matLocalTrans * matProj);
    mParticleBuffer.Update(matFinal);
    mColorBuffer.Update(color);
    mParticle.Render();
}

void ParticleSystemEffect::DebugUI()
{
}

void ParticleSystemEffect::SetCamera(const Camera& camera)
{
    mCamera = &camera;
}

void ParticleSystemEffect::SetTextureId(TextureId id)
{
    mTextureId = id;
}
