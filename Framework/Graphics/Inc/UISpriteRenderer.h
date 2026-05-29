#pragma once

namespace Engine::Graphics
{
class UISprite;

class UISpriteRenderer final
{
  public:
    static void StaticInitialize();
    static void StaticTerminate();
    static UISpriteRenderer* Get();

    UISpriteRenderer() = default;
    ~UISpriteRenderer() = default;

    void Initialize();
    void Terminate();

    void BeginRender();
    void EndRender();

    void Render(const UISprite& uiSprite);
};
} // namespace Engine::Graphics
