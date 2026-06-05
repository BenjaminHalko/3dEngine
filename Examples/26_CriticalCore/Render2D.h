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

// Logical font selector. Font maps to the fFont atlas (uppercase + digits HUD
// glyphs); Score maps to the fScore atlas (full ASCII score popups).
enum class Font2D
{
    Font,
    Score
};

// Horizontal anchor for DrawText. Vertical placement is the caller's via y.
enum class TextAlign
{
    Left,
    Center,
    Right
};

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

    // Fills the region OUTSIDE a convex polygon with a solid color, extruded to a
    // far bounding ring that covers the whole 256x224 view at any camera zoom.
    // `polygon` vertices are in 256x224 draw space, ordered around the shape.
    // This paints the BLACK VOID around the octagonal arena "hole": the original
    // (oBackground/Draw_0.gml) masks the scene to the octagon - inside is the
    // navy backdrop + game, everything outside is a solid black void. count must
    // be <= 12 so the doubled-winding ring fits the shared color mesh.
    void FillConvexExterior(const Math::Vector2* polygon, int count, const Graphics::Color& color);

    // ---- Bitmap text (task-4 baked atlases) ------------------------------
    // Loads one atlas PNG + its glyph-metrics JSON into the given Font2D slot.
    // atlasPng / glyphJson are paths relative to build/bin (e.g.
    // "Assets/Fonts/CriticalCore/fFont.png"). Safe to call again to replace.
    void LoadFont(Font2D font, const std::string& atlasPng, const std::string& glyphJson);

    // Draws text as tinted glyph quads through the sprite-quad path, advancing
    // the pen by each glyph's xadvance and offsetting by xoffset/yoffset. The
    // pen origin is (x, y) (top-left of the line cell); Center/Right shift the
    // start by MeasureText. Unknown chars advance by the space width, no draw.
    void DrawText(Font2D font,
                  const std::string& text,
                  float x,
                  float y,
                  const Graphics::Color& color,
                  TextAlign align = TextAlign::Left);

    // Total pen advance for text in the given font, matching the drawn width.
    float MeasureText(Font2D font, const std::string& text) const;

    // Filled n-sided disc drawn through the shCore volumetric shader, tinted by
    // `tint` and modulated by `intensity` (0 = raw nebula, 0.5 = bright Core).
    // (x,y) is DRAW-space; iTime is CoreComponent::EffectTime(). One shared
    // shader/mesh/cbuffer serves every caller. uv per GM oPlayer/Draw_0.gml.
    void DrawNebulaCircle(
        float x, float y, float radius, const Graphics::Color& tint, float intensity, float iTime);

    // Scene view zoom (oCamera scale 1.0..1.5): rebuilds the ortho as a zoom
    // about the screen centre so every draw tracks it; the nebula primitive
    // folds the same zoom in via GetViewScale().
    void SetViewScale(float scale);
    float GetViewScale() const
    {
        return mViewScale;
    }

  private:
    void InitializeNebula();
    void TerminateNebula();

    // Mirrors CriticalCore_Core.hlsl CoreBuffer (register b0). 48 bytes = 3 rows.
    struct NebulaData
    {
        float iTime = 0.0f;
        float iResX = 0.0f;
        float iResY = 0.0f;
        float iResZ = 0.0f;
        float intensity = 0.0f;
        float pad0 = 0.0f;
        float pad1 = 0.0f;
        float pad2 = 0.0f;
        float tintR = 1.0f;
        float tintG = 1.0f;
        float tintB = 1.0f;
        float tintA = 1.0f;
    };
    using NebulaBuffer = Graphics::TypedConstantBuffer<NebulaData>;

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

    // ---- Bitmap-font path ------------------------------------------------
    // One baked glyph: source sub-rect in atlas pixels plus pen metrics.
    struct Glyph
    {
        float u = 0.0f;
        float v = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
        float xoffset = 0.0f;
        float yoffset = 0.0f;
        float xadvance = 0.0f;
    };

    struct FontData
    {
        Graphics::TextureId texture = 0;
        bool loaded = false;
        float atlasWidth = 1.0f;
        float atlasHeight = 1.0f;
        float lineHeight = 0.0f;
        float spaceAdvance = 0.0f;
        std::unordered_map<int, Glyph> glyphs;
    };

    // Largest text submit before clamping: kMaxGlyphsPerDraw glyphs * 6 verts.
    static constexpr uint32_t kMaxGlyphsPerDraw = 256u;
    static constexpr uint32_t kMaxGlyphVertices = kMaxGlyphsPerDraw * 6u;

    const FontData& GetFont(Font2D font) const;

    // Appends the two-triangle quad (6 VertexPX) for one glyph at (gx,gy).
    void AppendGlyphQuad(std::vector<Graphics::VertexPX>& verts,
                         float gx,
                         float gy,
                         const Glyph& glyph,
                         const FontData& fontData) const;

    std::array<FontData, 2> mFonts;

    // Text path: reused dynamic VertexPX mesh, shares the sprite VS/PS, sampler
    // and SpriteData cbuffer (world identity, verts authored in pixel space).
    Graphics::MeshBuffer mGlyphMesh;

    // Nebula path: own shCore VS/PS + dynamic mesh + CoreBuffer, shared by all
    // callers (player blob + every bubble). Triangle fan, kNebulaSides sectors.
    static constexpr int kNebulaSides = 20;
    static constexpr uint32_t kNebulaVertices = static_cast<uint32_t>(kNebulaSides) * 3u;
    Graphics::VertexShader mNebulaVertexShader;
    Graphics::PixelShader mNebulaPixelShader;
    Graphics::MeshBuffer mNebulaMesh;
    NebulaBuffer mNebulaBuffer;

    // Current scene view zoom (1.0 = no zoom). Folded into mOrtho by SetViewScale
    // and into the nebula primitive's manual NDC.
    float mViewScale = 1.0f;
};
} // namespace Engine::CriticalCore
