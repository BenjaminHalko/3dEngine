#include "RenderTarget2D.h"

using namespace Engine;
using namespace Engine::Graphics;
using namespace Engine::CriticalCore;

void RenderTarget2D::Initialize(int internalW, int internalH)
{
    mInternalWidth = internalW;
    mInternalHeight = internalH;

    // Offscreen target at the native 256x224 internal resolution. RGBA_U8 is the
    // backbuffer-matching 8-bit format (the scene is LDR pixel-art, no HDR).
    mRenderTarget.Initialize(static_cast<uint32_t>(internalW),
                             static_cast<uint32_t>(internalH),
                             RenderTarget::Format::RGBA_U8);

    // Fullscreen quad in NDC (positions [-1,+1]); the upscale VS is a passthrough.
    MeshPX screenQuadMesh = MeshBuilder::CreateScreenQuadPX();
    mScreenQuad.Initialize(screenQuadMesh);

    // Self-contained upscale shaders (passthrough VS + texture-sampling PS).
    const std::filesystem::path shaderFile = L"Assets/Shaders/CriticalCore_Upscale.hlsl";
    mVertexShader.Initialize<VertexPX>(shaderFile);
    mPixelShader.Initialize(shaderFile);

    // POINT sampling only -> crisp pixel-art upscale (NO linear filtering).
    // Clamp avoids edge bleed when the upscaled quad samples the RT borders.
    mSampler.Initialize(Sampler::Filter::Point, Sampler::AddressMode::Clamp);

    // ---- Bloom: two ping-pong glow targets + the separable blur shader ----
    mGlowRT0.Initialize(static_cast<uint32_t>(internalW), static_cast<uint32_t>(internalH), RenderTarget::Format::RGBA_U8);
    mGlowRT1.Initialize(static_cast<uint32_t>(internalW), static_cast<uint32_t>(internalH), RenderTarget::Format::RGBA_U8);

    const std::filesystem::path brightFile = L"Assets/Shaders/CriticalCore_BrightExtract.hlsl";
    mBrightVertexShader.Initialize<VertexPX>(brightFile);
    mBrightPixelShader.Initialize(brightFile);
    mBrightBuffer.Initialize();

    const std::filesystem::path blurFile = L"Assets/Shaders/CriticalCore_Blur.hlsl";
    mBlurVertexShader.Initialize<VertexPX>(blurFile);
    mBlurPixelShader.Initialize(blurFile);
    mBlurBuffer.Initialize();

    // LINEAR for the glow (interpolated, unlike the POINT scene upscale).
    mLinearSampler.Initialize(Sampler::Filter::Linear, Sampler::AddressMode::Clamp);

    // Pure additive: dst += glow. The glow is bright-extracted (dark everywhere
    // except the bright core/title/walls), so adding it only blooms those.
    D3D11_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    GraphicsSystem::Get()->GetDevice()->CreateBlendState(&blendDesc, &mGlowBlendState);
}

void RenderTarget2D::Terminate()
{
    SafeRelease(mGlowBlendState);
    mLinearSampler.Terminate();
    mBlurBuffer.Terminate();
    mBlurPixelShader.Terminate();
    mBlurVertexShader.Terminate();
    mBrightBuffer.Terminate();
    mBrightPixelShader.Terminate();
    mBrightVertexShader.Terminate();
    mGlowRT1.Terminate();
    mGlowRT0.Terminate();

    mSampler.Terminate();
    mPixelShader.Terminate();
    mVertexShader.Terminate();
    mScreenQuad.Terminate();
    mRenderTarget.Terminate();
}

void RenderTarget2D::BeginScene(const Color& clear)
{
    // Binds the RT (+ its 256x224 viewport) and clears it. RenderTarget stashes
    // the previous render target/viewport for EndScene to restore.
    mRenderTarget.BeginRender(clear);
}

void RenderTarget2D::EndScene()
{
    mRenderTarget.EndRender();
}

RenderTarget2D::LetterboxRect RenderTarget2D::ComputeLetterbox(int windowW, int windowH) const
{
    LetterboxRect rect;

    const float winW = static_cast<float>(windowW);
    const float winH = static_cast<float>(windowH);
    const float internalW = static_cast<float>(mInternalWidth);
    const float internalH = static_cast<float>(mInternalHeight);

    // Largest uniform scale that fits the internal resolution inside the window
    // while preserving the internalW:internalH aspect (no stretch).
    const float scale = std::min(winW / internalW, winH / internalH);

    rect.scale = scale;
    rect.width = internalW * scale;
    rect.height = internalH * scale;
    // Center -> equal black bars on the letterboxed axis.
    rect.x = (winW - rect.width) * 0.5f;
    rect.y = (winH - rect.height) * 0.5f;

    return rect;
}

void RenderTarget2D::BloomPass()
{
    // Spread > 1 widens the 5-tap gaussian so the glow reads as a bloom halo
    // rather than a 5px smudge (the taps step spread*texel each).
    constexpr float kSpread = 2.5f;
    const float texelX = kSpread / static_cast<float>(mInternalWidth);
    const float texelY = kSpread / static_cast<float>(mInternalHeight);

    // Pass 0 - bright-extract the scene RT into mGlowRT0 (keep only bright px).
    mBrightVertexShader.Bind();
    mBrightPixelShader.Bind();
    mLinearSampler.BindPS(0);
    BrightData bright;
    bright.threshold = 0.35f;
    bright.knee = 0.25f;
    bright.intensity = 1.0f;
    mGlowRT0.BeginRender(Colors::Black);
    mBrightBuffer.Update(bright);
    mBrightBuffer.BindPS(0);
    mRenderTarget.BindPS(0);
    mScreenQuad.Render();
    Texture::UnbindPS(0);
    mGlowRT0.EndRender();

    // Pass 1 - horizontal blur of the extracted glow (mGlowRT0 -> mGlowRT1).
    mBlurVertexShader.Bind();
    mBlurPixelShader.Bind();
    mLinearSampler.BindPS(0);
    BlurData horizontal;
    horizontal.blurVector = {1.0f, 0.0f};
    horizontal.texelSize = {texelX, texelY};
    mGlowRT1.BeginRender(Colors::Black);
    mBlurBuffer.Update(horizontal);
    mBlurBuffer.BindPS(0);
    mGlowRT0.BindPS(0);
    mScreenQuad.Render();
    Texture::UnbindPS(0);
    mGlowRT1.EndRender();

    // Pass 2 - vertical blur (mGlowRT1 -> mGlowRT0). mGlowRT0 holds the bloom.
    BlurData vertical;
    vertical.blurVector = {0.0f, 1.0f};
    vertical.texelSize = {texelX, texelY};
    mGlowRT0.BeginRender(Colors::Black);
    mBlurBuffer.Update(vertical);
    mBlurBuffer.BindPS(0);
    mGlowRT1.BindPS(0);
    mScreenQuad.Render();
    Texture::UnbindPS(0);
    mGlowRT0.EndRender();
}

void RenderTarget2D::Present(int windowW, int windowH)
{
    // Blur the just-rendered scene into mGlowRT1 (additively composited below).
    BloomPass();

    const LetterboxRect rect = ComputeLetterbox(windowW, windowH);

    auto context = GraphicsSystem::Get()->GetContext();

    // Set a custom viewport == the letterbox rect. The fullscreen NDC quad maps
    // exactly onto this rect; anything outside keeps the (black) backbuffer
    // clear color -> letterbox bars. No vertex scaling needed.
    D3D11_VIEWPORT viewport{};
    viewport.TopLeftX = rect.x;
    viewport.TopLeftY = rect.y;
    viewport.Width = rect.width;
    viewport.Height = rect.height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    mVertexShader.Bind();
    mPixelShader.Bind();
    mSampler.BindPS(0);
    mRenderTarget.BindPS(0);

    mScreenQuad.Render();

    // Unbind the RT as a shader resource so it can be re-bound as a render
    // target next frame without a read/write hazard.
    Texture::UnbindPS(0);

    // mGlowRT0 holds the bright-extracted + blurred bloom; a pure ADD only
    // brightens the glowing core/title/walls and leaves the navy backdrop dark.
    context->OMSetBlendState(mGlowBlendState, nullptr, 0xffffffffu);
    mLinearSampler.BindPS(0);
    mGlowRT0.BindPS(0);
    mScreenQuad.Render();
    Texture::UnbindPS(0);
    context->OMSetBlendState(nullptr, nullptr, 0xffffffffu);

    // Restore the full-window viewport for subsequent passes (ImGui, etc.).
    GraphicsSystem::Get()->ResetViewport();
}
