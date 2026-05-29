#include "Precompiled.h"
#include "UIFont.h"

using namespace Engine;
using namespace Engine::Graphics;

namespace
{
std::unique_ptr<UIFont> sUIFont;

std::string WideToUtf8(const wchar_t* str)
{
    if (str == nullptr)
    {
        return {};
    }
    std::string out;
    for (const wchar_t* p = str; *p != L'\0'; ++p)
    {
        const uint32_t code = static_cast<uint32_t>(*p);
        if (code < 0x80)
        {
            out.push_back(static_cast<char>(code));
        }
        else if (code < 0x800)
        {
            out.push_back(static_cast<char>(0xC0 | (code >> 6)));
            out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
        }
        else if (code < 0x10000)
        {
            out.push_back(static_cast<char>(0xE0 | (code >> 12)));
            out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
        }
        else
        {
            out.push_back(static_cast<char>(0xF0 | (code >> 18)));
            out.push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
        }
    }
    return out;
}
} // namespace

void UIFont::StaticInitialize(FontType font)
{
    ASSERT(sUIFont == nullptr, "UIFont: Is already Initialized!");
    sUIFont = std::make_unique<UIFont>();
    sUIFont->Initialize(font);
}

void UIFont::StaticTerminate()
{
    if (sUIFont != nullptr)
    {
        sUIFont->Terminate();
        sUIFont.reset();
    }
}

UIFont* UIFont::Get()
{
    ASSERT(sUIFont != nullptr, "UIFont: Is not Initialized!");
    return sUIFont.get();
}

void UIFont::Initialize(FontType font)
{
    mFontType = font;
}

void UIFont::Terminate()
{
}

void UIFont::DrawString(const wchar_t* str,
                        const Math::Vector2& position,
                        const Color& color,
                        float size)
{
    const std::string utf8 = WideToUtf8(str);
    const ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(color.r, color.g, color.b, color.a));
    ImGui::GetForegroundDrawList()->AddText(
        ImGui::GetFont(), size, ImVec2(position.x, position.y), col, utf8.c_str());
}

float UIFont::GetStringWidth(const wchar_t* str, float size) const
{
    const std::string utf8 = WideToUtf8(str);
    const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(size, FLT_MAX, 0.0f, utf8.c_str());
    return textSize.x;
}
