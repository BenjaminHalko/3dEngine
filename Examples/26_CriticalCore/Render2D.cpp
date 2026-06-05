#include "Render2D.h"

#include "GmHelpers.h"

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>

#include <cmath>
#include <cstdio>
#include <vector>

using namespace Engine;
using namespace Engine::Graphics;
using namespace Engine::Math;

namespace Engine::CriticalCore
{
Matrix4 Ortho2D(int width, int height)
{
    const float w = static_cast<float>(width);
    const float h = static_cast<float>(height);

    // Row-vector (row-major) off-center orthographic, y-DOWN:
    //   x in [0, w] -> clip x in [-1, +1]
    //   y in [0, h] -> clip y in [+1, -1]  (top-left origin, +y downward)
    //   z passes through unchanged (quad sits at z = 0)
    return Matrix4(2.0f / w, 0.0f, 0.0f, 0.0f,
                   0.0f, -2.0f / h, 0.0f, 0.0f,
                   0.0f, 0.0f, 1.0f, 0.0f,
                   -1.0f, 1.0f, 0.0f, 1.0f);
}

void Render2D::Initialize()
{
    mOrtho = Ortho2D(kInternalWidth, kInternalHeight);

    // Unit quad centered on the origin (-0.5..0.5), scaled per draw.
    const MeshPX quad = MeshBuilder::CreateSpriteQuadPX(1.0f, 1.0f);
    mQuad.Initialize(quad);

    const std::filesystem::path shaderPath = L"Assets/Shaders/CriticalCore_Sprite.hlsl";
    mVertexShader.Initialize<VertexPX>(shaderPath);
    mPixelShader.Initialize(shaderPath);

    // Point/clamp: crisp pixel-art, no bleeding past sprite edges.
    mSampler.Initialize(Sampler::Filter::Point, Sampler::AddressMode::Clamp);

    mSpriteBuffer.Initialize();

    const std::filesystem::path colorShaderPath = L"Assets/Shaders/CriticalCore_Color.hlsl";
    mColorVertexShader.Initialize<VertexPC>(colorShaderPath);
    mColorPixelShader.Initialize(colorShaderPath);

    // Reused dynamic vertex buffer (nullptr init -> D3D11_USAGE_DYNAMIC), big
    // enough for the largest single submit; refilled per draw via Update().
    mColorMesh.Initialize(nullptr, static_cast<uint32_t>(sizeof(VertexPC)), kMaxColorVertices);
    mColorMesh.SetTopology(MeshBuffer::Topology::Triangles);

    mColorBuffer.Initialize();

    mGlyphMesh.Initialize(nullptr, static_cast<uint32_t>(sizeof(VertexPX)), kMaxGlyphVertices);
    mGlyphMesh.SetTopology(MeshBuffer::Topology::Triangles);

    InitializeNebula();
}

void Render2D::Terminate()
{
    TerminateNebula();

    mGlyphMesh.Terminate();

    mColorBuffer.Terminate();
    mColorPixelShader.Terminate();
    mColorVertexShader.Terminate();
    mColorMesh.Terminate();

    mSpriteBuffer.Terminate();
    mSampler.Terminate();
    mPixelShader.Terminate();
    mVertexShader.Terminate();
    mQuad.Terminate();
}

TextureId Render2D::LoadTexture(const std::string& path)
{
    return TextureManager::Get()->LoadTexture(path);
}

void Render2D::DrawSprite(TextureId tex,
                          float x,
                          float y,
                          float originX,
                          float originY,
                          float scaleX,
                          float scaleY,
                          float rotDeg,
                          const Color& tint)
{
    const Texture* texture = TextureManager::Get()->GetTexture(tex);
    if (texture == nullptr)
    {
        return;
    }

    const float w = static_cast<float>(texture->GetWidth());
    const float h = static_cast<float>(texture->GetHeight());

    // Map the centered unit quad into sprite-pixel space (top-left = (0,0),
    // bottom-right = (w,h), +y down). The quad's v=0 edge (local y = +0.5) maps
    // to py = 0 (texture top), so the y axis is flipped here.
    const Matrix4 baseToPixel = Matrix4::Scaling(w, -h, 1.0f) *
                                Matrix4::Translation(0.5f * w, 0.5f * h, 0.0f);

    // GameMaker draw_sprite_ext order: subtract origin, scale, rotate, translate.
    // Positive rotDeg is counter-clockwise on screen; under the y-down ortho that
    // corresponds to a negative RotationZ angle.
    const float rad = -rotDeg * Constants::DegToRad;

    const Matrix4 world = baseToPixel *
                          Matrix4::Translation(-originX, -originY, 0.0f) *
                          Matrix4::Scaling(scaleX, scaleY, 1.0f) *
                          Matrix4::RotationZ(rad) *
                          Matrix4::Translation(x, y, 0.0f);

    mVertexShader.Bind();
    mPixelShader.Bind();
    mSampler.BindPS(0);

    SpriteData data;
    data.wvp = Transpose(world * mOrtho);
    data.tint = tint;
    mSpriteBuffer.Update(data);
    mSpriteBuffer.BindVS(0);
    mSpriteBuffer.BindPS(0);

    texture->BindPS(0);
    mQuad.Render();
}

namespace
{
// HelperFunctions.gml:114 -> _offset = 0.5 + (!BROWSER and !OPERA)*0.5. On the
// native desktop target (neither a browser nor Opera) that resolves to 1.0px.
// The original GameMaker drawCircle drew FILLED circles shifted +1,+1 from their
// start position; under our top-left/y-DOWN coordinate handling we reproduce the
// match by shifting filled circles -1,-1. This applies to DrawCircleFilled ONLY
// (player blob, bubbles, core HP disc, particles) - NOT outlines, lines, sprites,
// or text, which have no such offset in the source.
constexpr float kFilledCircleOffset = 1.0f;
} // namespace

void Render2D::AppendSegmentQuad(std::vector<VertexPC>& verts,
                                 float ax,
                                 float ay,
                                 float bx,
                                 float by,
                                 float thickness,
                                 const Color& color) const
{
    const float dx = bx - ax;
    const float dy = by - ay;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-6f)
    {
        return;
    }

    // Unit perpendicular (-dy,dx)/len, scaled to half-thickness, gives the two
    // offset edges of the quad.
    const float h = thickness * 0.5f;
    const float nx = (-dy / len) * h;
    const float ny = (dx / len) * h;

    const Vector3 p0(ax + nx, ay + ny, 0.0f);
    const Vector3 p1(bx + nx, by + ny, 0.0f);
    const Vector3 p2(bx - nx, by - ny, 0.0f);
    const Vector3 p3(ax - nx, ay - ny, 0.0f);

    // Wind clockwise in NDC to match the filled-circle fan: under the y-DOWN ortho
    // (which flips y) the naive p0,p1,p2 order is counter-clockwise = a back face,
    // so the default CullMode=BACK rasterizer culls every line/ring. Emit
    // p0,p2,p1 / p0,p3,p2 so segment quads are front-facing and actually draw.
    verts.push_back({p0, color});
    verts.push_back({p2, color});
    verts.push_back({p1, color});
    verts.push_back({p0, color});
    verts.push_back({p3, color});
    verts.push_back({p2, color});
}

void Render2D::DrawRing(float cx, float cy, float radius, float thickness, const Color& color)
{
    std::vector<VertexPC> verts;
    verts.reserve(static_cast<size_t>(kCircleSlices) * 6);

    for (int i = 0; i < kCircleSlices; ++i)
    {
        const float a0 = (static_cast<float>(i) / kCircleSlices) * Constants::TwoPi;
        const float a1 = (static_cast<float>(i + 1) / kCircleSlices) * Constants::TwoPi;
        const float ax = cx + radius * std::cos(a0);
        const float ay = cy + radius * std::sin(a0);
        const float bx = cx + radius * std::cos(a1);
        const float by = cy + radius * std::sin(a1);
        AppendSegmentQuad(verts, ax, ay, bx, by, thickness, color);
    }

    SubmitColorMesh(verts);
}

void Render2D::SubmitColorMesh(const std::vector<VertexPC>& verts)
{
    if (verts.empty())
    {
        return;
    }

    mColorVertexShader.Bind();
    mColorPixelShader.Bind();

    ColorData data;
    data.wvp = Transpose(mOrtho);
    mColorBuffer.Update(data);
    mColorBuffer.BindVS(0);

    mColorMesh.Update(verts.data(), static_cast<uint32_t>(verts.size()));
    mColorMesh.Render();
}

void Render2D::DrawCircleFilled(float x, float y, float radius, const Color& color, bool outline)
{
    const float cx = x - kFilledCircleOffset;
    const float cy = y - kFilledCircleOffset;

    // Filled disc as a triangle list (D3D11 has no triangle-fan topology):
    // each slice is (center, p_i, p_{i+1}).
    std::vector<VertexPC> verts;
    verts.reserve(static_cast<size_t>(kCircleSlices) * 3);

    const Vector3 center(cx, cy, 0.0f);
    for (int i = 0; i < kCircleSlices; ++i)
    {
        const float a0 = (static_cast<float>(i) / kCircleSlices) * Constants::TwoPi;
        const float a1 = (static_cast<float>(i + 1) / kCircleSlices) * Constants::TwoPi;
        const Vector3 p0(cx + radius * std::cos(a0), cy + radius * std::sin(a0), 0.0f);
        const Vector3 p1(cx + radius * std::cos(a1), cy + radius * std::sin(a1), 0.0f);
        verts.push_back({center, color});
        verts.push_back({p0, color});
        verts.push_back({p1, color});
    }
    SubmitColorMesh(verts);

    if (outline)
    {
        // drawCircle bubble branch (:121-128): outline = fill hue/value with
        // saturation reduced to 30%. MakeColorHSV forces alpha 1, so restore the
        // fill's alpha afterwards.
        Color outlineColor = MakeColorHSV(ColorGetHue(color), ColorGetSat(color) * 0.3f, ColorGetValue(color));
        outlineColor.a = color.a;
        DrawRing(cx, cy, radius, 1.0f, outlineColor);
    }
}

void Render2D::DrawCircleOutline(float x, float y, float radius, const Color& color)
{
    // No pixel offset here: only FILLED circles (DrawCircleFilled) carry the
    // -1,-1 shift. Outlines draw at their exact requested position.
    DrawRing(x, y, radius, 1.0f, color);
}

void Render2D::DrawLine(float x0, float y0, float x1, float y1, float thickness, const Color& color)
{
    std::vector<VertexPC> verts;
    verts.reserve(6);
    AppendSegmentQuad(verts, x0, y0, x1, y1, thickness, color);
    SubmitColorMesh(verts);
}

const Render2D::FontData& Render2D::GetFont(Font2D font) const
{
    return mFonts[static_cast<size_t>(font)];
}

void Render2D::LoadFont(Font2D font, const std::string& atlasPng, const std::string& glyphJson)
{
    FontData& fontData = mFonts[static_cast<size_t>(font)];
    fontData.glyphs.clear();
    fontData.loaded = false;

    // The atlas lives under Assets/Fonts, not the TextureManager root
    // (Assets/Textures), so bypass the root-dir prefix.
    fontData.texture = TextureManager::Get()->LoadTexture(atlasPng, false);

    FILE* file = nullptr;
    const errno_t err = fopen_s(&file, glyphJson.c_str(), "r");
    if (err != 0 || file == nullptr)
    {
        return;
    }

    char readBuffer[65536];
    rapidjson::FileReadStream readStream(file, readBuffer, sizeof(readBuffer));
    rapidjson::Document doc;
    doc.ParseStream(readStream);
    fclose(file);

    if (doc.HasParseError() || !doc.IsObject())
    {
        return;
    }

    fontData.atlasWidth = static_cast<float>(doc["atlasWidth"].GetInt());
    fontData.atlasHeight = static_cast<float>(doc["atlasHeight"].GetInt());
    fontData.lineHeight = static_cast<float>(doc["lineHeight"].GetInt());

    const auto& glyphArray = doc["glyphs"];
    for (const auto& entry : glyphArray.GetArray())
    {
        const int code = entry["char"].GetInt();
        Glyph glyph;
        glyph.u = static_cast<float>(entry["u"].GetInt());
        glyph.v = static_cast<float>(entry["v"].GetInt());
        glyph.w = static_cast<float>(entry["w"].GetInt());
        glyph.h = static_cast<float>(entry["h"].GetInt());
        glyph.xoffset = static_cast<float>(entry["xoffset"].GetInt());
        glyph.yoffset = static_cast<float>(entry["yoffset"].GetInt());
        glyph.xadvance = static_cast<float>(entry["xadvance"].GetInt());
        fontData.glyphs[code] = glyph;
    }

    // Space (ASCII 32) advance doubles as the fallback for missing glyphs.
    const auto spaceIt = fontData.glyphs.find(32);
    fontData.spaceAdvance = (spaceIt != fontData.glyphs.end()) ? spaceIt->second.xadvance : 0.0f;

    fontData.loaded = true;
}

float Render2D::MeasureText(Font2D font, const std::string& text) const
{
    const FontData& fontData = GetFont(font);
    if (!fontData.loaded)
    {
        return 0.0f;
    }

    float width = 0.0f;
    for (const char ch : text)
    {
        const int code = static_cast<unsigned char>(ch);
        const auto it = fontData.glyphs.find(code);
        width += (it != fontData.glyphs.end()) ? it->second.xadvance : fontData.spaceAdvance;
    }
    return width;
}

void Render2D::AppendGlyphQuad(std::vector<VertexPX>& verts,
                               float gx,
                               float gy,
                               const Glyph& glyph,
                               const FontData& fontData) const
{
    // Normalized atlas sub-rect, inset half a texel on every edge so point
    // sampling at the quad borders stays strictly inside this glyph's cell and
    // never picks up a white texel from the neighbouring atlas cell. Source
    // pixels are y-DOWN like the ortho, so v increases downward (no flip).
    constexpr float inset = 0.5f;
    const float u0 = (glyph.u + inset) / fontData.atlasWidth;
    const float v0 = (glyph.v + inset) / fontData.atlasHeight;
    const float u1 = (glyph.u + glyph.w - inset) / fontData.atlasWidth;
    const float v1 = (glyph.v + glyph.h - inset) / fontData.atlasHeight;

    const Vector3 topLeft(gx, gy, 0.0f);
    const Vector3 topRight(gx + glyph.w, gy, 0.0f);
    const Vector3 bottomLeft(gx, gy + glyph.h, 0.0f);
    const Vector3 bottomRight(gx + glyph.w, gy + glyph.h, 0.0f);

    verts.push_back({topLeft, {u0, v0}});
    verts.push_back({topRight, {u1, v0}});
    verts.push_back({bottomRight, {u1, v1}});
    verts.push_back({topLeft, {u0, v0}});
    verts.push_back({bottomRight, {u1, v1}});
    verts.push_back({bottomLeft, {u0, v1}});
}

void Render2D::DrawText(
    Font2D font, const std::string& text, float x, float y, const Color& color, TextAlign align)
{
    const FontData& fontData = GetFont(font);
    if (!fontData.loaded)
    {
        return;
    }

    const Texture* texture = TextureManager::Get()->GetTexture(fontData.texture);
    if (texture == nullptr)
    {
        return;
    }

    float penX = x;
    if (align == TextAlign::Center)
    {
        penX = x - MeasureText(font, text) * 0.5f;
    }
    else if (align == TextAlign::Right)
    {
        penX = x - MeasureText(font, text);
    }

    std::vector<VertexPX> verts;
    verts.reserve(text.size() * 6);

    for (const char ch : text)
    {
        if (verts.size() >= kMaxGlyphVertices)
        {
            break;
        }

        const int code = static_cast<unsigned char>(ch);
        const auto it = fontData.glyphs.find(code);
        if (it == fontData.glyphs.end())
        {
            penX += fontData.spaceAdvance;
            continue;
        }

        const Glyph& glyph = it->second;
        const bool isSpace = (code == 32);
        const bool emptyCell = (glyph.w <= 0.0f || glyph.h <= 0.0f);
        if (!isSpace && !emptyCell)
        {
            AppendGlyphQuad(verts, penX + glyph.xoffset, y + glyph.yoffset, glyph, fontData);
        }
        penX += glyph.xadvance;
    }

    if (verts.empty())
    {
        return;
    }

    mVertexShader.Bind();
    mPixelShader.Bind();
    mSampler.BindPS(0);

    SpriteData data;
    data.wvp = Transpose(mOrtho);
    data.tint = color;
    mSpriteBuffer.Update(data);
    mSpriteBuffer.BindVS(0);
    mSpriteBuffer.BindPS(0);

    texture->BindPS(0);
    mGlyphMesh.Update(verts.data(), static_cast<uint32_t>(verts.size()));
    mGlyphMesh.Render();
}

void Render2D::InitializeNebula()
{
    const std::filesystem::path coreShaderPath = L"Assets/Shaders/CriticalCore_Core.hlsl";
    mNebulaVertexShader.Initialize<VertexPX>(coreShaderPath);
    mNebulaPixelShader.Initialize(coreShaderPath);
    mNebulaBuffer.Initialize();
    mNebulaMesh.Initialize(nullptr, static_cast<uint32_t>(sizeof(VertexPX)), kNebulaVertices);
    mNebulaMesh.SetTopology(MeshBuffer::Topology::Triangles);
}

void Render2D::TerminateNebula()
{
    mNebulaMesh.Terminate();
    mNebulaBuffer.Terminate();
    mNebulaPixelShader.Terminate();
    mNebulaVertexShader.Terminate();
}

void Render2D::SetViewScale(float scale)
{
    mViewScale = (scale > 1.0e-4f) ? scale : 1.0f;

    // Zoom the y-DOWN ortho about the screen centre (internalW/2, internalH/2):
    // a point P maps to clip = 2*(P - centre)/(res*scale), so the view shows
    // res*scale of the room centred on the screen middle.
    const float w = static_cast<float>(kInternalWidth);
    const float h = static_cast<float>(kInternalHeight);
    mOrtho = Matrix4(2.0f / (w * mViewScale), 0.0f, 0.0f, 0.0f,
                     0.0f, -2.0f / (h * mViewScale), 0.0f, 0.0f,
                     0.0f, 0.0f, 1.0f, 0.0f,
                     -1.0f / mViewScale, 1.0f / mViewScale, 0.0f, 1.0f);
}

void Render2D::DrawNebulaCircle(
    float x, float y, float radius, const Color& tint, float intensity, float iTime)
{
    if (radius <= 0.0f)
    {
        return;
    }

    const float w = static_cast<float>(kInternalWidth);
    const float h = static_cast<float>(kInternalHeight);
    const float cx = w * 0.5f;
    const float cy = h * 0.5f;

    // Fold the same view zoom the ortho uses (this primitive emits NDC directly,
    // bypassing the ortho), so the nebula tracks the camera zoom with everything else.
    auto toNdcX = [&](float sx) { return (((sx - cx) / mViewScale + cx) / w) * 2.0f - 1.0f; };
    auto toNdcY = [&](float sy) { return 1.0f - (((sy - cy) / mViewScale + cy) / h) * 2.0f; };

    std::vector<VertexPX> verts;
    verts.reserve(kNebulaVertices);

    const Vector3 centerPos(toNdcX(x), toNdcY(y), 0.0f);
    const Vector2 centerUv(0.5f, 0.5f);

    for (int i = 0; i < kNebulaSides; ++i)
    {
        const float a0 = (static_cast<float>(i) / kNebulaSides) * Constants::TwoPi;
        const float a1 = (static_cast<float>(i + 1) / kNebulaSides) * Constants::TwoPi;
        const float dx0 = radius * std::cos(a0);
        const float dy0 = radius * std::sin(a0);
        const float dx1 = radius * std::cos(a1);
        const float dy1 = radius * std::sin(a1);

        verts.push_back({centerPos, centerUv});
        verts.push_back({{toNdcX(x + dx0), toNdcY(y + dy0), 0.0f}, {0.5f + dx0 / w, 0.5f + dy0 / h}});
        verts.push_back({{toNdcX(x + dx1), toNdcY(y + dy1), 0.0f}, {0.5f + dx1 / w, 0.5f + dy1 / h}});
    }

    mNebulaMesh.Update(verts.data(), static_cast<uint32_t>(verts.size()));

    NebulaData data;
    data.iTime = iTime;
    data.iResX = w;
    data.iResY = h;
    data.iResZ = 0.0f;
    data.intensity = intensity;
    data.tintR = tint.r;
    data.tintG = tint.g;
    data.tintB = tint.b;
    data.tintA = tint.a;

    mNebulaVertexShader.Bind();
    mNebulaPixelShader.Bind();
    mNebulaBuffer.Update(data);
    mNebulaBuffer.BindPS(0);
    mNebulaMesh.Render();
}
} // namespace Engine::CriticalCore
