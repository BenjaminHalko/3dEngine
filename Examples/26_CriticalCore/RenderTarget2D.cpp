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

    // Self-contained upscale shaders (passthrough VS + tinted texture-sampling PS).
    const std::filesystem::path shaderFile = L"Assets/Shaders/CriticalCore_Upscale.hlsl";
    mVertexShader.Initialize<VertexPX>(shaderFile);
    mPixelShader.Initialize(shaderFile);
    mUpscaleBuffer.Initialize();

    // POINT sampling only -> crisp pixel-art upscale (NO linear filtering).
    // Clamp avoids edge bleed when the upscaled quad samples the RT borders.
    mSampler.Initialize(Sampler::Filter::Point, Sampler::AddressMode::Clamp);

    // ---- Full-scene blur: two ping-pong targets + the separable shBlur shader ----
    mPongRT.Initialize(static_cast<uint32_t>(internalW), static_cast<uint32_t>(internalH), RenderTarget::Format::RGBA_U8);
    mBlurRT.Initialize(static_cast<uint32_t>(internalW), static_cast<uint32_t>(internalH), RenderTarget::Format::RGBA_U8);

    const std::filesystem::path blurFile = L"Assets/Shaders/CriticalCore_Blur.hlsl";
    mBlurVertexShader.Initialize<VertexPX>(blurFile);
    mBlurPixelShader.Initialize(blurFile);
    mBlurBuffer.Initialize();

    // LINEAR for the blur taps (interpolates between texels, like the GM default
    // sampler in shBlur), unlike the POINT scene upscale.
    mLinearSampler.Initialize(Sampler::Filter::Linear, Sampler::AddressMode::Clamp);

    // dst += src: the additive sharp-scene pass (GM gpu_set_blendmode(bm_add)).
    D3D11_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    GraphicsSystem::Get()->GetDevice()->CreateBlendState(&blendDesc, &mAddBlendState);
}

void RenderTarget2D::Terminate()
{
    SafeRelease(mAddBlendState);
    mLinearSampler.Terminate();
    mBlurBuffer.Terminate();
    mBlurPixelShader.Terminate();
    mBlurVertexShader.Terminate();
    mBlurRT.Terminate();
    mPongRT.Terminate();

    mUpscaleBuffer.Terminate();
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

void RenderTarget2D::BlurScene()
{
    // GM shBlur steps one internal-res texel per tap (no spread): texel = 1/res.
    BlurData blur;
    blur.texelSize = {1.0f / static_cast<float>(mInternalWidth),
                      1.0f / static_cast<float>(mInternalHeight)};

    mBlurVertexShader.Bind();
    mBlurPixelShader.Bind();
    mLinearSampler.BindPS(0);

    // PASS 1 (GM Draw_64:6-8) - vertical blur of the scene into the pong target.
    blur.blurVector = {0.0f, 1.0f};
    mPongRT.BeginRender(Colors::Black);
    mBlurBuffer.Update(blur);
    mBlurBuffer.BindPS(0);
    mRenderTarget.BindPS(0);
    mScreenQuad.Render();
    Texture::UnbindPS(0);
    mPongRT.EndRender();

    // PASS 2 (GM Draw_64:11) - horizontal blur of the pong -> fully blurred scene.
    blur.blurVector = {1.0f, 0.0f};
    mBlurRT.BeginRender(Colors::Black);
    mBlurBuffer.Update(blur);
    mBlurBuffer.BindPS(0);
    mPongRT.BindPS(0);
    mScreenQuad.Render();
    Texture::UnbindPS(0);
    mBlurRT.EndRender();
}

void RenderTarget2D::Present(int windowW, int windowH)
{
    // mBlurRT = HBlur(VBlur(scene)): the soft full-scene blur (no bright-extract).
    BlurScene();

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

    // Both composite draws use the POINT upscale shader with a 0.7 grey tint
    // (GM make_color_hsv(0,0,255*0.7)). The GM order is blurred-base (normal) then
    // sharp (additive); since the scene is opaque, normal-blend equals an opaque
    // write, so the two draws are reordered to sharp-opaque-base + blurred-additive
    // (addition commutes) — identical pixels, drawn in the proven upscale order.
    UpscaleData tint;
    tint.tint = {0.7f, 0.7f, 0.7f, 1.0f};
    mVertexShader.Bind();
    mPixelShader.Bind();
    mUpscaleBuffer.Update(tint);
    mUpscaleBuffer.BindPS(0);
    mSampler.BindPS(0);

    // BASE (GM Draw_64:16 term) - the sharp scene * 0.7, opaque.
    context->OMSetBlendState(nullptr, nullptr, 0xffffffffu);
    mRenderTarget.BindPS(0);
    mScreenQuad.Render();
    Texture::UnbindPS(0);

    // GLOW (GM Draw_64:12 term) - the blurred scene * 0.7, additive (bm_add).
    context->OMSetBlendState(mAddBlendState, nullptr, 0xffffffffu);
    mBlurRT.BindPS(0);
    mScreenQuad.Render();
    Texture::UnbindPS(0);
    context->OMSetBlendState(nullptr, nullptr, 0xffffffffu);

    // Restore the full-window viewport for subsequent passes (ImGui, etc.).
    GraphicsSystem::Get()->ResetViewport();
}
