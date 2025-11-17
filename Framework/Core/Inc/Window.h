#pragma once

namespace Engine::Core
{
class Window
{
  public:
    // Platform-agnostic initialize (instance ignored on non-Windows)
    void Initialize(void* instance,
                    const std::wstring& appName,
                    uint32_t width,
                    uint32_t height);

    void Terminate();

    void ProcessMessage();

    GLFWwindow* GetWindowHandle() const;

    bool IsActive() const;

  private:
    GLFWwindow* mWindow = nullptr;
    std::wstring mAppName;
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    bool mIsActive = false;
};
} // namespace Engine::Core
