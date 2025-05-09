#pragma once

namespace Engine::Core {
    class Window {
    public:
        void Initialize(HINSTANCE instance, const std::wstring& appName, uint32_t width, uint32_t height);
        void Terminate();

        void ProcessMessage();

        [[nodiscard]] HWND GetWindowHandle() const;
        [[nodiscard]] bool IsActive() const;
    private:
        HINSTANCE mInstance = nullptr;
        HWND mWindow = nullptr;
        RECT mScreenRect{};
        std::wstring mAppName;
        bool mIsActive = false;
    };
}
