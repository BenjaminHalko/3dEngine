#include "precompiled.h"
#include "window_message_handler.h"
#include "debugutil.h"

using namespace Engine::Core;

void WindowMessageHandler::Hook(HWND window, Callback callback) {
    mWindow = window;
    mPreviousCallback = reinterpret_cast<Callback>(GetWindowLongPtrA(window, GWLP_WNDPROC));
    SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(callback));
}

void WindowMessageHandler::Unhook() {
    SetWindowLongPtrA(mWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(mPreviousCallback));
    mWindow = nullptr;
}

LRESULT WindowMessageHandler::ForwardMessage(HWND window, const UINT message, const WPARAM wParam, const LPARAM lParam) const {
    ASSERT(mPreviousCallback != nullptr, "WindowMessageHandler::ForwardMessage called with nullptr");
    return CallWindowProcA(mPreviousCallback, window, message, wParam, lParam);
}
