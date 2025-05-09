#include "precompiled.h"
#include "window_message_handler.h"
#include "debugutil.h"

using namespace Engine::Core;

void WindowMessageHandler::Hook(HWND window, Callback callback) {
    mWindow = window;
    mPreviousCallback = reinterpret_cast<Callback>(GetWindowLongPtrW(window, GWLP_WNDPROC));
    SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(callback));
}

void WindowMessageHandler::Unhook() {
    SetWindowLongPtrW(mWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(mPreviousCallback));
    mWindow = nullptr;
}

LRESULT WindowMessageHandler::ForwardMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    ASSERT(mPreviousCallback != nullptr, "WindowMessageHandler::ForwardMessage called with nullptr");
    return CallWindowProcW(mPreviousCallback, window, message, wParam, lParam);
}
