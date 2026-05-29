#pragma once

#include "UIComponent.h"

namespace Engine
{
class UISpriteComponent : public UIComponent
{
  public:
    SET_TYPE_ID(ComponentId::UISprite);

    void Initialize() override;
    void Terminate() override;
    void Render() override;
    void Deserialize(const rapidjson::Value& value) override;

    Math::Vector2 GetPosition(bool includeOrigin = true);

  private:
    std::filesystem::path mTexturePath;
    Math::Vector2 mPosition = Math::Vector2::Zero;
    Graphics::UISprite::Rect mRect{0, 0, 0, 0};
    Graphics::UISprite mUISprite;
};
} // namespace Engine
