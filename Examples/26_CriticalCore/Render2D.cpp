#include "Render2D.h"

#include "GmHelpers.h"

#include <cmath>
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
}

void Render2D::Terminate()
{
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
constexpr float kPixelOffset = 1.0f;
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

    verts.push_back({p0, color});
    verts.push_back({p1, color});
    verts.push_back({p2, color});
    verts.push_back({p0, color});
    verts.push_back({p2, color});
    verts.push_back({p3, color});
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
    const float cx = x - kPixelOffset;
    const float cy = y - kPixelOffset;

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
    DrawRing(x - kPixelOffset, y - kPixelOffset, radius, 1.0f, color);
}

void Render2D::DrawLine(float x0, float y0, float x1, float y1, float thickness, const Color& color)
{
    std::vector<VertexPC> verts;
    verts.reserve(6);
    AppendSegmentQuad(verts, x0, y0, x1, y1, thickness, color);
    SubmitColorMesh(verts);
}
} // namespace Engine::CriticalCore
