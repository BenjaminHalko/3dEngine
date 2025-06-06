#include "Window.h"
#include "Precompiled.h"

using namespace Engine;
using namespace Engine::Core;

LRESULT CALLBACK WindowMessageHandler(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }
    default:
        break;
    }

    return DefWindowProc(handle, msg, wParam, lParam);
}

void Window::Initialize(HINSTANCE instance, const std::wstring &appName, uint32_t width,
                        uint32_t height) {
    mInstance = instance;
    mAppName = appName;

    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowMessageHandler;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = instance;
    wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_CROSS);
    wcex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = mAppName.c_str();

    RegisterClassEx(&wcex);

    mScreenRect = {0, 0, (LONG)width, (LONG)height};
    AdjustWindowRect(&mScreenRect, WS_OVERLAPPEDWINDOW, FALSE);

    const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    const int winWidth =
        std::min(static_cast<int>(mScreenRect.right - mScreenRect.left), screenWidth);
    const int winHeight =
        std::min(static_cast<int>(mScreenRect.bottom - mScreenRect.top), screenHeight);

    const int left = (screenWidth - winWidth) / 2;
    const int top = (screenHeight - winHeight) / 2;

    mScreenRect.left = left;
    mScreenRect.top = top;

    mWindow = CreateWindow(mAppName.c_str(), mAppName.c_str(), WS_OVERLAPPEDWINDOW, left, top,
                           winWidth, winHeight, nullptr, nullptr, instance, nullptr);

    ShowWindow(mWindow, SW_SHOWNORMAL);
    SetCursorPos(screenWidth / 2, screenHeight / 2);
    mIsActive = (mWindow != nullptr);
}

void Window::Terminate() {
    DestroyWindow(mWindow);
    UnregisterClass(mAppName.c_str(), mInstance);
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

HWND Window::GetWindowHandle() const { return mWindow; }

bool Window::IsActive() const { return mIsActive; }
