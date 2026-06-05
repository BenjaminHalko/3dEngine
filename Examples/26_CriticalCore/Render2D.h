#pragma once

#include <Engine/Inc/Engine.h>

namespace Engine::CriticalCore
{
// ---------------------------------------------------------------------------
// Render2D - the base 2D sprite-draw layer for Critical Core 2.
//
// Mirrors GameMaker's draw_sprite_ext semantics on top of the 3dEngine
// Graphics layer: an orthographic, y-DOWN camera over the 256x224 internal
// resolution (top-left origin, +y downward) so source coordinates map 1:1 to
// the original game. Draws a unit textured quad scaled per-draw to the bound
// texture's pixel dimensions, tinted by a per-draw RGBA color.
//
// This is the foundation other render passes extend:
//   - task 11 (circle / line primitives)
//   - task 12 (bitmap text)
//   - task 13 (render target + integer upscale to the 768x672 backbuffer)
// Those tasks MUST reuse this class's ortho convention, cbuffer layout
// (register b0: float4x4 wvp + float4 tint), and runtime shader-compile idiom.
//
// Render2D does NOT bind a render target or touch the backbuffer; it issues
// draws against whatever target is currently bound (task 13 owns the RT).
// ---------------------------------------------------------------------------

// Default internal resolution (Critical Core 2 source space).
constexpr int kInternalWidth = 256;
constexpr int kInternalHeight = 224;

// Builds the y-DOWN orthographic projection for a given internal resolution:
// x in [0, width]  -> clip x in [-1, +1]
// y in [0, height] -> clip y in [+1, -1]   (top-left origin, +y downward)
// Returned in row-vector (row-major) form; transpose before GPU upload.
Math::Matrix4 Ortho2D(int width = kInternalWidth, int height = kInternalHeight);

class Render2D final
{
  public:
    void Initialize();
    void Terminate();

    // Thin wrapper over TextureManager so callers load via task-2 manifest
    // paths relative to Assets/Textures, e.g. "CriticalCore/sCore_0.png".
    Graphics::TextureId LoadTexture(const std::string& path);

    // GameMaker draw_sprite_ext analogue. The quad is sized to the texture's
    // native pixel dimensions, then placed so the sprite pixel (originX,originY)
    // lands at (x, y). rotDeg is in DEGREES (GameMaker convention).
    void DrawSprite(Graphics::TextureId tex,
                    float x,
                    float y,
                    float originX,
                    float originY,
                    float scaleX,
                    float scaleY,
                    float rotDeg,
                    const Graphics::Color& tint);

  private:
    struct SpriteData
    {
        Math::Matrix4 wvp;
        Graphics::Color tint;
    };

    using SpriteBuffer = Graphics::TypedConstantBuffer<SpriteData>;

    Math::Matrix4 mOrtho = Math::Matrix4::Identity;

    Graphics::MeshBuffer mQuad;
    Graphics::VertexShader mVertexShader;
    Graphics::PixelShader mPixelShader;
    Graphics::Sampler mSampler;
    SpriteBuffer mSpriteBuffer;
};
} // namespace Engine::CriticalCore
