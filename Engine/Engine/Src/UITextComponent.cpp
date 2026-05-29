#include "Precompiled.h"
#include "UITextComponent.h"

#include "GameObject.h"
#include "GameWorld.h"
#include "UIRenderService.h"
#include "SaveUtil.h"

using namespace Engine;
using namespace Engine::Graphics;

void UITextComponent::Initialize()
{
    UIRenderService* uiRenderService = GetOwner().GetWorld().GetService<UIRenderService>();
    ASSERT(uiRenderService != nullptr, "UITextComponent: Needs a UI render service!");
    uiRenderService->Register(this);
}

void UITextComponent::Terminate()
{
    UIRenderService* uiRenderService = GetOwner().GetWorld().GetService<UIRenderService>();
    if (uiRenderService != nullptr)
    {
        uiRenderService->Unregister(this);
    }
}

void UITextComponent::Render()
{
    UIFont::Get()->DrawString(mText.wstring().c_str(), mPosition, mColor, mSize);
}

void UITextComponent::Deserialize(const rapidjson::Value& value)
{
    if (value.HasMember("Text"))
    {
        mText = value["Text"].GetString();
    }
    if (value.HasMember("Position"))
    {
        auto pos = value["Position"].GetArray();
        mPosition.x = pos[0].GetFloat();
        mPosition.y = pos[1].GetFloat();
    }
    SaveUtil::ReadColor("Color", mColor, value);
    SaveUtil::ReadFloat("Size", mSize, value);
}
