#pragma once

namespace Engine {
    struct AppConfig {
        std::wstring appName = L"AppName";
        uint32_t width = 1200;
        uint32_t height = 720;
    };

    class App final {
        bool mRunning = false;
    public:
        void Run(const AppConfig& config);
        void Quit();
    };
}
