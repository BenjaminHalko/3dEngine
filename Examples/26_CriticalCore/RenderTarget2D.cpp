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

    // Final upscale shader (single-texture POINT passthrough + tint).
    const std::filesystem::path shaderFile = L"Assets/Shaders/CriticalCore_Upscale.hlsl";
    mVertexShader.Initialize<VertexPX>(shaderFile);
    mPixelShader.Initialize(shaderFile);
    mUpscaleBuffer.Initialize();

    // Bloom composite shader (two-texture: scene POINT + blur LINEAR).
    const std::filesystem::path bloomFile = L"Assets/Shaders/CriticalCore_Bloom.hlsl";
    mBloomVertexShader.Initialize<VertexPX>(bloomFile);
    mBloomPixelShader.Initialize(bloomFile);

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
}

void RenderTarget2D::Terminate()
{
    mLinearSampler.Terminate();
    mBlurBuffer.Terminate();
    mBlurPixelShader.Terminate();
    mBlurVertexShader.Terminate();
    mBlurRT.Terminate();
    mPongRT.Terminate();

    mBloomPixelShader.Terminate();
    mBloomVertexShader.Terminate();
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

void RenderTarget2D::BloomAndBeginUI()
{
    // mBlurRT = HBlur(VBlur(scene)): the soft full-world blur (no bright-extract).
    // mPongRT held the VBlur intermediate and is now free to reuse below.
    BlurScene();

    auto context = GraphicsSystem::Get()->GetContext();

    // Composite the bloomed world into mPongRT at native 256x224, OPAQUE:
    //   mPongRT = (scene[POINT] + blur[LINEAR]) * 0.7   (GM make_color_hsv(0,0,255*0.7)).
    // Faithful Draw_64: sharp world *0.7 + blurred world *0.7 summed in one PS so
    // no additive backbuffer blend is needed (that renders unreliably on dxmt).
    UpscaleData tint;
    tint.tint = {0.7f, 0.7f, 0.7f, 1.0f};

    mPongRT.BeginRender(Colors::Black);
    context->OMSetBlendState(nullptr, nullptr, 0xffffffffu);

    mBloomVertexShader.Bind();
    mBloomPixelShader.Bind();
    mUpscaleBuffer.Update(tint);
    mUpscaleBuffer.BindPS(0);

    mRenderTarget.BindPS(0);
    mSampler.BindPS(0);
    mBlurRT.BindPS(1);
    mLinearSampler.BindPS(1);
    mScreenQuad.Render();
    Texture::UnbindPS(0);
    Texture::UnbindPS(1);

    // Leave mPongRT bound (256x224 viewport): the render service now draws the
    // screen-space UI SHARP on top of this bloomed world, before EndUIAndPresent
    // unbinds it and point-upscales the composite to the window.
}

void RenderTarget2D::EndUIAndPresent(int windowW, int windowH)
{
    // Unbind mPongRT (restores the backbuffer); it now holds bloomed world + UI.
    mPongRT.EndRender();

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

    // Final upscale: POINT-sample the composite (mPongRT) at a 1.0 passthrough
    // tint -> crisp pixel-art (the bloom's 0.7 grey lives in the composite).
    UpscaleData tint;
    tint.tint = {1.0f, 1.0f, 1.0f, 1.0f};
    mVertexShader.Bind();
    mPixelShader.Bind();
    mUpscaleBuffer.Update(tint);
    mUpscaleBuffer.BindPS(0);

    context->OMSetBlendState(nullptr, nullptr, 0xffffffffu);
    mPongRT.BindPS(0);
    mSampler.BindPS(0);
    mScreenQuad.Render();
    Texture::UnbindPS(0);

    // Restore the full-window viewport for subsequent passes (ImGui, etc.).
    GraphicsSystem::Get()->ResetViewport();
}
