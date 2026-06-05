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

    // ---- Color primitives (VertexPC, no texture) -------------------------
    // All coordinates are in 256x224 source-pixel space (same ortho as the
    // sprite path). These reproduce HelperFunctions.gml drawCircle /
    // drawCircleOutline (:113-137): the GameMaker pixel-center offset is baked
    // into the circle center, and the optional filled-circle outline uses a
    // reduced-saturation copy of the fill color (GmHelpers HSV).

    // GameMaker draw_circle(filled). When outline=true, also strokes a 1px ring
    // in the desaturated fill color (drawCircle bubble branch, :117-128).
    void DrawCircleFilled(float x, float y, float radius, const Graphics::Color& color, bool outline = false);

    // GameMaker draw_circle(outline-only): a 1px ring in the given color, no
    // desaturation (drawCircleOutline, :134-137).
    void DrawCircleOutline(float x, float y, float radius, const Graphics::Color& color);

    // Thick line as a single quad from (x0,y0) to (x1,y1).
    void DrawLine(float x0, float y0, float x1, float y1, float thickness, const Graphics::Color& color);

  private:
    struct SpriteData
    {
        Math::Matrix4 wvp;
        Graphics::Color tint;
    };

    using SpriteBuffer = Graphics::TypedConstantBuffer<SpriteData>;

    // Color-path cbuffer: just the transposed ortho (world is identity because
    // color geometry is authored directly in source-pixel space). 64 bytes,
    // already 16-byte aligned.
    struct ColorData
    {
        Math::Matrix4 wvp;
    };

    using ColorBuffer = Graphics::TypedConstantBuffer<ColorData>;

    // Triangle-fan slice count for the filled circle and the outline ring.
    static constexpr int kCircleSlices = 24;
    // Largest single submit: the outline ring = kCircleSlices segment quads * 6
    // verts. The filled fan (kCircleSlices * 3) and a line (6) are both smaller.
    static constexpr uint32_t kMaxColorVertices = static_cast<uint32_t>(kCircleSlices) * 6u;

    // Appends one thickness-wide quad (two triangles) spanning (ax,ay)->(bx,by).
    void AppendSegmentQuad(std::vector<Graphics::VertexPC>& verts,
                           float ax,
                           float ay,
                           float bx,
                           float by,
                           float thickness,
                           const Graphics::Color& color) const;

    // Strokes a 1px-thick ring of kCircleSlices segment quads about (cx,cy).
    void DrawRing(float cx, float cy, float radius, float thickness, const Graphics::Color& color);

    // Binds the color shader/cbuffer, uploads verts into the reused dynamic
    // mesh, and issues the draw. No-op on empty input.
    void SubmitColorMesh(const std::vector<Graphics::VertexPC>& verts);

    Math::Matrix4 mOrtho = Math::Matrix4::Identity;

    Graphics::MeshBuffer mQuad;
    Graphics::VertexShader mVertexShader;
    Graphics::PixelShader mPixelShader;
    Graphics::Sampler mSampler;
    SpriteBuffer mSpriteBuffer;

    // Color path: reused dynamic mesh (no per-frame realloc) + dedicated VS/PS
    // and wvp cbuffer.
    Graphics::MeshBuffer mColorMesh;
    Graphics::VertexShader mColorVertexShader;
    Graphics::PixelShader mColorPixelShader;
    ColorBuffer mColorBuffer;
};
} // namespace Engine::CriticalCore
