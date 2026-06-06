#pragma once

#include <Engine/Inc/Engine.h>

namespace Engine::CriticalCore
{
// ---------------------------------------------------------------------------
// RenderTarget2D - the 256x224 offscreen render target + point-sampled upscale.
//
// Critical Core 2 is a 256x224 pixel-art game presented in a 768x672 (3x)
// window. To keep every pixel crisp and the source coordinate space 1:1, the
// entire 2D scene (Render2D draws, services, GUI) is rendered into an internal
// 256x224 RenderTarget, then a single fullscreen pass upscales it to the window
// using a POINT sampler (no linear filtering) and an aspect-correct letterbox
// (black bars; never stretched).
//
// Usage (CriticalCore2DRenderService::Render, two-phase so the screen-space UI
// is NOT blurred by the bloom):
//   mRT.Initialize();
//   ...
//   mRT.BeginScene(Colors::Black);          // bind scene RT + clear
//       <draw WORLD (non-screen-space) components + void mask, 1:1 in 256x224>
//   mRT.EndScene();
//   mRT.BloomAndBeginUI();                  // bloom the world, composite it into
//                                           //   the UI buffer, leave it bound
//       <draw UI (screen-space) components SHARP on top of the bloomed world>
//   mRT.EndUIAndPresent(winW, winH);        // unbind + letterboxed POINT upscale
//
// Self-contained: this class owns its RenderTargets, screen-quad, POINT/LINEAR
// samplers, the separable blur (CriticalCore_Blur.hlsl), the bloom composite
// (CriticalCore_Bloom.hlsl) and the final upscale (CriticalCore_Upscale.hlsl).
// It does NOT touch Render2D or GameState.
// ---------------------------------------------------------------------------

class RenderTarget2D final
{
  public:
    // Aspect-correct letterbox rectangle (in backbuffer pixels) used by Present.
    struct LetterboxRect
    {
        float x = 0.0f;      // top-left X offset (black bar width on the left)
        float y = 0.0f;      // top-left Y offset (black bar height on top)
        float width = 0.0f;  // upscaled draw width  (internalW * scale)
        float height = 0.0f; // upscaled draw height (internalH * scale)
        float scale = 1.0f;  // uniform scale = min(winW/internalW, winH/internalH)
    };

    void Initialize(int internalW = 256, int internalH = 224);
    void Terminate();

    // Bind the internal RenderTarget and clear it. All subsequent draws land in
    // the internalW x internalH space until EndScene().
    void BeginScene(const Graphics::Color& clear = Graphics::Colors::Black);

    // Unbind the internal RenderTarget (restores the previous target/viewport).
    void EndScene();

    // Bloom the WORLD that was drawn between BeginScene/EndScene: run the
    // separable blur, composite (scene + blur) * 0.7 into the UI buffer at native
    // 256x224, and LEAVE that buffer bound so the caller can draw the sharp
    // screen-space UI on top of the bloomed world. Pair with EndUIAndPresent.
    void BloomAndBeginUI();

    // Unbind the UI buffer and draw it (bloomed world + sharp UI) to the window
    // with a POINT-sampled, aspect-correct letterbox upscale. Restores the
    // full-window viewport afterwards so later passes (ImGui, etc.) render
    // normally. Pair with BloomAndBeginUI.
    void EndUIAndPresent(int windowW, int windowH);

    // Pure letterbox math (preserve internalW:internalH aspect, center). Exposed
    // for callers/tests that need the rect without issuing a draw.
    LetterboxRect ComputeLetterbox(int windowW, int windowH) const;

    // Accessors for the underlying offscreen target (e.g. ImGui debug preview).
    Graphics::RenderTarget& GetRenderTarget()
    {
        return mRenderTarget;
    }
    const Graphics::RenderTarget& GetRenderTarget() const
    {
        return mRenderTarget;
    }

    int GetInternalWidth() const
    {
        return mInternalWidth;
    }
    int GetInternalHeight() const
    {
        return mInternalHeight;
    }

  private:
    // Faithful reproduction of GM oRender Draw_64: the WORLD scene RT is blurred
    // via two separable shBlur passes (vertical then horizontal) into mBlurRT.
    // There is NO bright-extract / threshold — the full world is blurred. The
    // composite (BloomAndBeginUI) is (scene + blur) * 0.7 in one opaque pass.
    void BlurScene();

    // Mirrors CriticalCore_Blur.hlsl BlurBuffer (register b0): pass direction in
    // texels + 1/internalRes texel size. 16 bytes (one float4 register).
    struct BlurData
    {
        Math::Vector2 blurVector;
        Math::Vector2 texelSize;
    };
    using BlurBuffer = Graphics::TypedConstantBuffer<BlurData>;

    // Per-draw RGBA tint shared by both b0-tint shaders: the bloom composite
    // (CriticalCore_Bloom.hlsl) uses 0.7 grey (GM make_color_hsv(0,0,255*0.7));
    // the final upscale (CriticalCore_Upscale.hlsl) uses 1.0 (passthrough).
    struct UpscaleData
    {
        Math::Vector4 tint{1.0f, 1.0f, 1.0f, 1.0f};
    };
    using UpscaleBuffer = Graphics::TypedConstantBuffer<UpscaleData>;

    int mInternalWidth = 256;
    int mInternalHeight = 224;

    Graphics::RenderTarget mRenderTarget;
    Graphics::MeshBuffer mScreenQuad;
    Graphics::VertexShader mVertexShader;
    Graphics::PixelShader mPixelShader;
    Graphics::Sampler mSampler;
    UpscaleBuffer mUpscaleBuffer;

    // Bloom composite shader (CriticalCore_Bloom.hlsl): two-texture opaque pass
    // that sums the sharp world (POINT) and its blurred copy (LINEAR).
    Graphics::VertexShader mBloomVertexShader;
    Graphics::PixelShader mBloomPixelShader;

    // Full-scene blur resources (two separable shBlur passes, no bright-extract).
    //   mPongRT = VBlur(scene), then REUSED as the bloom+UI composite buffer
    //             (the bloomed world is drawn here, the sharp UI on top of it).
    //   mBlurRT = HBlur(VBlur(scene))        (the fully blurred world)
    Graphics::RenderTarget mPongRT;
    Graphics::RenderTarget mBlurRT;
    Graphics::VertexShader mBlurVertexShader;
    Graphics::PixelShader mBlurPixelShader;
    BlurBuffer mBlurBuffer;
    Graphics::Sampler mLinearSampler;
};
} // namespace Engine::CriticalCore
