#include "Precompiled.h"
#include "UISpriteComponent.h"

#include "GameObject.h"
#include "GameWorld.h"
#include "UIRenderService.h"
#include "UIButtonComponent.h"

#include "SaveUtil.h"

using namespace Engine;
using namespace Engine::Graphics;

void UISpriteComponent::Initialize()
{
    ASSERT(!mTexturePath.empty(), "UISpriteComponent: Texture path is not set!");
    mUISprite.Initialize(mTexturePath);
    if (mRect.right > mRect.left)
    {
        mUISprite.SetRect(mRect.top, mRect.left, mRect.right, mRect.bottom);
    }
    UIRenderService* uiRenderService = GetOwner().GetWorld().GetService<UIRenderService>();
    ASSERT(uiRenderService != nullptr, "UISpriteComponent: Needs a UI render service!");
    uiRenderService->Register(this);
}

void UISpriteComponent::Terminate()
{
    UIRenderService* uiRenderService = GetOwner().GetWorld().GetService<UIRenderService>();
    if (uiRenderService != nullptr)
    {
        uiRenderService->Unregister(this);
    }
    mUISprite.Terminate();
}

void UISpriteComponent::Render()
{
    Math::Vector2 worldPos = GetPosition(false);
    GameObject* parent = GetOwner().GetParent();
    while (parent != nullptr)
    {
        UISpriteComponent* spriteComponent = parent->GetComponent<UISpriteComponent>();
        if (spriteComponent != nullptr)
        {
            worldPos += spriteComponent->GetPosition();
        }
        else
        {
            UIButtonComponent* buttonComponent = parent->GetComponent<UIButtonComponent>();
            if (buttonComponent != nullptr)
            {
                worldPos += buttonComponent->GetPosition();
            }
        }

        parent = parent->GetParent();
    }

    mUISprite.SetPosition({worldPos.x, worldPos.y});
    UISpriteRenderer::Get()->Render(mUISprite);
}

void UISpriteComponent::Deserialize(const rapidjson::Value& value)
{
    if (value.HasMember("Texture"))
    {
        mTexturePath = value["Texture"].GetString();
    }
    if (value.HasMember("Position"))
    {
        auto pos = value["Position"].GetArray();
        mPosition.x = pos[0].GetFloat();
        mPosition.y = pos[1].GetFloat();
    }
    if (value.HasMember("Scale"))
    {
        auto scale = value["Scale"].GetArray();
        const float x = scale[0].GetFloat();
        const float y = scale[1].GetFloat();
        mUISprite.SetScale({x, y});
    }
    if (value.HasMember("Rect"))
    {
        auto rect = value["Rect"].GetArray();
        mRect.top = rect[0].GetInt();
        mRect.left = rect[1].GetInt();
        mRect.right = rect[2].GetInt();
        mRect.bottom = rect[3].GetInt();
    }
    if (value.HasMember("Pivot"))
    {
        std::string pivot = value["Pivot"].GetString();
        if (pivot == "TopLeft")
        {
            mUISprite.SetPivot(Pivot::TopLeft);
        }
        else if (pivot == "Top")
        {
            mUISprite.SetPivot(Pivot::Top);
        }
        else if (pivot == "TopRight")
        {
            mUISprite.SetPivot(Pivot::TopRight);
        }
        else if (pivot == "CentreLeft")
        {
            mUISprite.SetPivot(Pivot::Left);
        }
        else if (pivot == "Centre")
        {
            mUISprite.SetPivot(Pivot::Centre);
        }
        else if (pivot == "CentreRight")
        {
            mUISprite.SetPivot(Pivot::Right);
        }
        else if (pivot == "BottomLeft")
        {
            mUISprite.SetPivot(Pivot::BottomLeft);
        }
        else if (pivot == "Bottom")
        {
            mUISprite.SetPivot(Pivot::Bottom);
        }
        else if (pivot == "BottomRight")
        {
            mUISprite.SetPivot(Pivot::BottomRight);
        }
        else
        {
            ASSERT(false, "UISpriteComponent: Invalid pivot %s!", pivot.c_str());
        }
    }
    if (value.HasMember("Flip"))
    {
        std::string flip = value["Flip"].GetString();
        if (flip == "None")
        {
            mUISprite.SetFlip(Flip::None);
        }
        else if (flip == "Horizontal")
        {
            mUISprite.SetFlip(Flip::Horizontal);
        }
        else if (flip == "Vertical")
        {
            mUISprite.SetFlip(Flip::Vertical);
        }
        else if (flip == "Both")
        {
            mUISprite.SetFlip(Flip::Both);
        }
        else
        {
            ASSERT(false, "UISpriteComponent: Invalid flip %s!", flip.c_str());
        }
    }

    Color color = Colors::White;
    SaveUtil::ReadColor("Color", color, value);
    mUISprite.SetColor(color);

    float rotation = 0.0f;
    SaveUtil::ReadFloat("Rotation", rotation, value);
    mUISprite.SetRotation(rotation);
}

Math::Vector2 UISpriteComponent::GetPosition(bool includeOrigin)
{
    float x = mPosition.x;
    float y = mPosition.y;
    if (includeOrigin)
    {
        mUISprite.GetOrigin(x, y);
    }
    return {mPosition.x - x, mPosition.y - y};
}
