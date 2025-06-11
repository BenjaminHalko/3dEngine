#include "Precompiled.h"

#include "Camera.h"
#include "SimpleTextureEffect.h"
#include "VertexTypes.h"

using namespace Engine;
using namespace Engine::Graphics;

void SimpleTextureEffect::Initialize() {
    std::filesystem::path shaderPath = L"../../Assets/Shaders/DoTexture.fx";
    mVertexShader.Initialize<VertexPX>(shaderPath);
    mPixelShader.Initialize(shaderPath);
    mSampler.Initialize(Sampler::Filter::Linear, Sampler::AddressMode::Wrap);
    mTransformBuffer.Initialize(sizeof(Math::Matrix4));
}

void SimpleTextureEffect::Terminate() {
    mTransformBuffer.Terminate();
    mSampler.Terminate();
    mPixelShader.Terminate();
    mVertexShader.Terminate();
}

void SimpleTextureEffect::Begin() {
    mVertexShader.Bind();
    mPixelShader.Bind();
    mSampler.BindPS(0);
    mTransformBuffer.BindVS(0);
}

void SimpleTextureEffect::End() { Texture::UnbindPS(0); }

void SimpleTextureEffect::Render(const SimpleTextureEffect::RenderData &renderData) {
    ASSERT(mCamera != nullptr, "SimpleTextureEffect: Must have a camera!");
    const Math::Matrix4 matView = mCamera->GetViewMatrix();
    const Math::Matrix4 matProj = mCamera->GetProjectionMatrix();
    const Math::Matrix4 matFinal = renderData.matWorld * matView * matProj;
    const Math::Matrix4 wvp = Math::Transpose(matFinal);
    mTransformBuffer.Update(&wvp);

    TextureManager::Get()->BindPS(renderData.textureId, 0);
    renderData.mesh.Render();
}

void SimpleTextureEffect::SetCamera(const Camera &camera) { mCamera = &camera; }