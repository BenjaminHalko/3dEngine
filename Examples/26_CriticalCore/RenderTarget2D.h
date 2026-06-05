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
// Usage (wired by task 34, GameState):
//   mRT.Initialize();                       // 256x224 by default
//   ...
//   mRT.BeginScene(Colors::Black);          // bind RT + clear
//       render2D.DrawSprite(...);           // all gameplay draws land 1:1
//       service.Render();                   //   in the 256x224 RT space
//   mRT.EndScene();                         // restore previous target/viewport
//   mRT.Present(winW, winH);                // letterboxed POINT upscale to window
//
// Self-contained: this class owns its RenderTarget, screen-quad, POINT sampler,
// and upscale VS/PS (Assets/Shaders/CriticalCore_Upscale.hlsl). It does NOT
// touch Render2D or GameState.
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

    // Draw a fullscreen quad sampling the internal RenderTarget with POINT
    // filtering, scaled to the largest aspect-correct fit inside windowW x
    // windowH and centered (letterboxed). Restores the full-window viewport
    // afterwards so later passes (ImGui, etc.) render normally.
    void Present(int windowW, int windowH);

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
    // Faithful reproduction of GM oRender Draw_64: the WHOLE scene is blurred via
    // two separable shBlur passes (vertical then horizontal) into mBlurRT. There
    // is NO bright-extract / threshold — the full scene is blurred. The composite
    // (in Present) is: blurred * 0.7 (opaque base) + scene * 0.7 (additive), so
    // the result is a soft glow over the whole scene, brightest where the scene
    // is bright (because of the additive sharp copy), not a thresholded bloom.
    void BlurScene();

    // Mirrors CriticalCore_Blur.hlsl BlurBuffer (register b0): pass direction in
    // texels + 1/internalRes texel size. 16 bytes (one float4 register).
    struct BlurData
    {
        Math::Vector2 blurVector;
        Math::Vector2 texelSize;
    };
    using BlurBuffer = Graphics::TypedConstantBuffer<BlurData>;

    // Mirrors CriticalCore_Upscale.hlsl UpscaleBuffer (register b0): a per-draw
    // RGBA tint. The composite draws both the blurred base and the additive sharp
    // copy with a 0.7 grey tint (GM make_color_hsv(0,0,255*0.7)).
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

    // Full-scene blur resources (two separable shBlur passes, no bright-extract).
    //   mPongRT = VBlur(scene)               (the GM "surfacePong")
    //   mBlurRT = HBlur(VBlur(scene))        (the fully blurred scene)
    Graphics::RenderTarget mPongRT;
    Graphics::RenderTarget mBlurRT;
    Graphics::VertexShader mBlurVertexShader;
    Graphics::PixelShader mBlurPixelShader;
    BlurBuffer mBlurBuffer;
    Graphics::Sampler mLinearSampler;
    // Additive (One/One) blend state for the sharp-scene add pass.
    ID3D11BlendState* mAddBlendState = nullptr;
};
} // namespace Engine::CriticalCore
