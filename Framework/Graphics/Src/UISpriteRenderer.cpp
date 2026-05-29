#include "Precompiled.h"
#include "UISpriteRenderer.h"
#include "UISprite.h"

#include "TextureManager.h"

using namespace Engine;
using namespace Engine::Graphics;

namespace
{
std::unique_ptr<UISpriteRenderer> sUISpriteRenderer;
}

void UISpriteRenderer::StaticInitialize()
{
    ASSERT(sUISpriteRenderer == nullptr, "UISpriteRenderer: Is already initialized!");
    sUISpriteRenderer = std::make_unique<UISpriteRenderer>();
    sUISpriteRenderer->Initialize();
}

void UISpriteRenderer::StaticTerminate()
{
    if (sUISpriteRenderer != nullptr)
    {
        sUISpriteRenderer->Terminate();
        sUISpriteRenderer.reset();
    }
}

UISpriteRenderer* UISpriteRenderer::Get()
{
    ASSERT(sUISpriteRenderer != nullptr, "UISpriteRenderer: Is not initialized!");
    return sUISpriteRenderer.get();
}

void UISpriteRenderer::Initialize()
{
}

void UISpriteRenderer::Terminate()
{
}

void UISpriteRenderer::BeginRender()
{
}

void UISpriteRenderer::EndRender()
{
}

void UISpriteRenderer::Render(const UISprite& uiSprite)
{
    const Texture* texture = TextureManager::Get()->GetTexture(uiSprite.mTextureId);
    if (texture == nullptr)
    {
        return;
    }

    void* texId = texture->GetRawData();
    if (texId == nullptr)
    {
        return;
    }

    const float spriteWidth = static_cast<float>(uiSprite.mRect.right - uiSprite.mRect.left);
    const float spriteHeight = static_cast<float>(uiSprite.mRect.bottom - uiSprite.mRect.top);
    const float scaledWidth = spriteWidth * uiSprite.mScale.x;
    const float scaledHeight = spriteHeight * uiSprite.mScale.y;

    const float originX = uiSprite.mOrigin.x * uiSprite.mScale.x;
    const float originY = uiSprite.mOrigin.y * uiSprite.mScale.y;

    const float localL = -originX;
    const float localT = -originY;
    const float localR = scaledWidth - originX;
    const float localB = scaledHeight - originY;

    const float cosR = std::cos(uiSprite.mRotation);
    const float sinR = std::sin(uiSprite.mRotation);
    const float px = uiSprite.mPosition.x;
    const float py = uiSprite.mPosition.y;

    auto rotate = [&](float lx, float ly) -> ImVec2
    { return ImVec2(px + (lx * cosR - ly * sinR), py + (lx * sinR + ly * cosR)); };

    const ImVec2 corner_tl = rotate(localL, localT);
    const ImVec2 corner_tr = rotate(localR, localT);
    const ImVec2 corner_br = rotate(localR, localB);
    const ImVec2 corner_bl = rotate(localL, localB);

    const Texture* tex = texture;
    const float texW = static_cast<float>(tex->GetWidth());
    const float texH = static_cast<float>(tex->GetHeight());
    const float u0 = static_cast<float>(uiSprite.mRect.left) / texW;
    const float v0 = static_cast<float>(uiSprite.mRect.top) / texH;
    const float u1 = static_cast<float>(uiSprite.mRect.right) / texW;
    const float v1 = static_cast<float>(uiSprite.mRect.bottom) / texH;

    ImVec2 uv_tl(u0, v0);
    ImVec2 uv_tr(u1, v0);
    ImVec2 uv_br(u1, v1);
    ImVec2 uv_bl(u0, v1);

    switch (uiSprite.mFlip)
    {
    case Flip::Horizontal:
        std::swap(uv_tl, uv_tr);
        std::swap(uv_bl, uv_br);
        break;
    case Flip::Vertical:
        std::swap(uv_tl, uv_bl);
        std::swap(uv_tr, uv_br);
        break;
    case Flip::Both:
        std::swap(uv_tl, uv_br);
        std::swap(uv_tr, uv_bl);
        break;
    case Flip::None:
    default:
        break;
    }

    const ImU32 col = ImGui::ColorConvertFloat4ToU32(
        ImVec4(uiSprite.mColor.r, uiSprite.mColor.g, uiSprite.mColor.b, uiSprite.mColor.a));

    ImGui::GetForegroundDrawList()->AddImageQuad(reinterpret_cast<ImTextureID>(texId),
                                                 corner_tl,
                                                 corner_tr,
                                                 corner_br,
                                                 corner_bl,
                                                 uv_tl,
                                                 uv_tr,
                                                 uv_br,
                                                 uv_bl,
                                                 col);
}
