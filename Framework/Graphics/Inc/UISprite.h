#pragma once

#include "Color.h"
#include "TextureManager.h"

namespace Engine::Graphics
{
enum class Pivot
{
    TopLeft,
    Top,
    TopRight,
    Left,
    Centre,
    Right,
    BottomLeft,
    Bottom,
    BottomRight
};

enum class Flip
{
    None,
    Horizontal,
    Vertical,
    Both
};

class UISprite
{
  public:
    struct Rect
    {
        uint32_t top = 0;
        uint32_t left = 0;
        uint32_t right = 0;
        uint32_t bottom = 0;
    };

    UISprite() = default;
    ~UISprite();

    void Initialize(const std::filesystem::path& filePath);
    void Terminate();

    void SetPosition(const Math::Vector2& position);
    void SetScale(const Math::Vector2& scale);
    void SetRect(uint32_t top, uint32_t left, uint32_t right, uint32_t bottom);
    void SetPivot(Pivot pivot);
    void SetFlip(Flip flip);
    void SetColor(const Color& color);
    void SetRotation(float rotation);

    bool IsInSprite(float x, float y) const;
    void GetOrigin(float& x, float& y);

  private:
    void UpdateOrigin();

    friend class UISpriteRenderer;

    TextureId mTextureId = 0;
    Rect mRect{0, 0, 100, 100};
    Math::Vector2 mPosition{0.0f, 0.0f};
    Math::Vector2 mOrigin{0.0f, 0.0f};
    Math::Vector2 mScale{1.0f, 1.0f};
    Color mColor = Colors::White;
    float mRotation = 0.0f;
    Pivot mPivot = Pivot::Centre;
    Flip mFlip = Flip::None;
};
} // namespace Engine::Graphics
