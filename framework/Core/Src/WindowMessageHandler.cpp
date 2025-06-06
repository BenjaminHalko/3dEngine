#include "WindowMessageHandler.h"
#include "DebugUtil.h"
#include "Precompiled.h"

using namespace Engine;
using namespace Engine::Core;

void WindowMessageHandler::Hook(HWND window, Callback cb) {
    mWindow = window;
    mPreviousCallback = (Callback)GetWindowLongPtrA(window, GWLP_WNDPROC);
    SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR)cb);
}

void WindowMessageHandler::Unhook() {
    SetWindowLongPtrA(mWindow, GWLP_WNDPROC, (LONG_PTR)mPreviousCallback);
    mWindow = nullptr;
}

LRESULT WindowMessageHandler::ForwardMessage(HWND window, UINT message, WPARAM wParam,
                                             LPARAM lParam) {
    ASSERT(mPreviousCallback != nullptr, "WindowMessageHandler: no callback is hooked");
    return CallWindowProcA((WNDPROC)mPreviousCallback, window, message, wParam, lParam);
}
