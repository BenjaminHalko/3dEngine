#include "precompiled.h"
#include "window.h"

using namespace Engine::Core;

LRESULT CALLBACK DefaultMessageHandler(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY: {
            PostQuitMessage(0);
        } break;
        default: break;
    }

    return DefWindowProc(handle, msg, wParam, lParam);
}

void Window::Initialize(HINSTANCE instance, const std::wstring &appName, uint32_t width, uint32_t height) {
    mInstance = instance;
    mAppName = appName;

    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefaultMessageHandler;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = reinterpret_cast<LPCSTR>(mAppName.c_str());

    RegisterClassEx(&wc);

    mScreenRect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    AdjustWindowRect(&mScreenRect, WS_OVERLAPPEDWINDOW, false);

    const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    const int winWidth = std::min(static_cast<int>(mScreenRect.right - mScreenRect.left), screenWidth);
    const int winHeight = std::min(static_cast<int>(mScreenRect.bottom - mScreenRect.top), screenHeight);
    const int left = (screenWidth - winWidth) / 2;
    const int top = (screenHeight - winHeight) / 2;

    mScreenRect.left = left;
    mScreenRect.top = top;

    mWindow = CreateWindow(
        reinterpret_cast<LPCSTR>(mAppName.c_str()),
        reinterpret_cast<LPCSTR>(mAppName.c_str()),
        WS_OVERLAPPEDWINDOW,
        left, top,
        winWidth, winHeight,
        nullptr, nullptr,
        instance, nullptr
    );

    ShowWindow(mWindow, SW_SHOW);
    SetCursorPos(screenWidth / 2, screenHeight / 2);
    mIsActive = mWindow != nullptr;
}

void Window::Terminate() {
    DestroyWindow(mWindow);
    UnregisterClass(reinterpret_cast<LPCSTR>(mAppName.c_str()), mInstance);
    mWindow = nullptr;
    mInstance = nullptr;
    mIsActive = false;
}

void Window::ProcessMessage() {
    MSG msg{};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (WM_QUIT == msg.message) {
            mIsActive = false;
        }
    }
}

HWND Window::GetWindowHandle() const {
    return mWindow;
}

bool Window::IsActive() const {
    return mIsActive;
}
