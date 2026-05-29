#include "Precompiled.h"
#include "UISprite.h"

using namespace Engine;
using namespace Engine::Graphics;

UISprite::~UISprite()
{
    ASSERT(mTextureId == 0, "UISprite: Terminate must be called!");
}

void UISprite::Initialize(const std::filesystem::path& filePath)
{
    TextureManager* tm = TextureManager::Get();
    mTextureId = tm->LoadTexture(filePath);
    const Texture* texture = tm->GetTexture(mTextureId);
    ASSERT(texture != nullptr, "UISprite: Failed to load texture %s!", filePath.string().c_str());
    SetRect(0, 0, texture->GetWidth(), texture->GetHeight());
}

void UISprite::Terminate()
{
    TextureManager::Get()->ReleaseTexture(mTextureId);
    mTextureId = 0;
}

void UISprite::SetPosition(const Math::Vector2& position)
{
    mPosition = position;
}

void UISprite::SetScale(const Math::Vector2& scale)
{
    mScale = scale;
}

void UISprite::SetRect(uint32_t top, uint32_t left, uint32_t right, uint32_t bottom)
{
    mRect.top = top;
    mRect.left = left;
    mRect.right = right;
    mRect.bottom = bottom;
    UpdateOrigin();
}

void UISprite::SetPivot(Pivot pivot)
{
    mPivot = pivot;
    UpdateOrigin();
}

void UISprite::SetFlip(Flip flip)
{
    mFlip = flip;
}

void UISprite::SetColor(const Color& color)
{
    mColor = color;
}

void UISprite::SetRotation(float rotation)
{
    mRotation = rotation;
}

bool UISprite::IsInSprite(float x, float y) const
{
    const float width = static_cast<float>(mRect.right - mRect.left);
    const float height = static_cast<float>(mRect.bottom - mRect.top);
    return x >= mPosition.x - mOrigin.x && x <= mPosition.x + width - mOrigin.x &&
           y >= mPosition.y - mOrigin.y && y <= mPosition.y + height - mOrigin.y;
}

void UISprite::GetOrigin(float& x, float& y)
{
    x = mOrigin.x;
    y = mOrigin.y;
}

namespace
{
constexpr Math::Vector2 gOffsets[] = {
    {0.0f, 0.0f}, // TopLeft
    {0.5f, 0.0f}, // Top
    {1.0f, 0.0f}, // TopRight
    {0.0f, 0.5f}, // Left
    {0.5f, 0.5f}, // Centre
    {1.0f, 0.5f}, // Right
    {0.0f, 1.0f}, // BottomLeft
    {0.5f, 1.0f}, // Bottom
    {1.0f, 1.0f}, // BottomRight
};
}

void UISprite::UpdateOrigin()
{
    const float width = static_cast<float>(mRect.right - mRect.left);
    const float height = static_cast<float>(mRect.bottom - mRect.top);
    const auto index = static_cast<std::underlying_type_t<Pivot>>(mPivot);

    mOrigin = {width * gOffsets[index].x, height * gOffsets[index].y};
}
