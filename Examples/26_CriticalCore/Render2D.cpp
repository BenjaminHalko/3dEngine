#include "Render2D.h"

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
}

void Render2D::Terminate()
{
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
} // namespace Engine::CriticalCore
