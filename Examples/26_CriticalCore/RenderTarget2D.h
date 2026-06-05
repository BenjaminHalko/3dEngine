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
    int mInternalWidth = 256;
    int mInternalHeight = 224;

    Graphics::RenderTarget mRenderTarget;
    Graphics::MeshBuffer mScreenQuad;
    Graphics::VertexShader mVertexShader;
    Graphics::PixelShader mPixelShader;
    Graphics::Sampler mSampler;
};
} // namespace Engine::CriticalCore
