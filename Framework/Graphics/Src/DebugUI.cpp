#include "Precompiled.h"
#include "DebugUI.h"

#include "GraphicsSystem.h"

#include <ImGui/inc/imgui_impl_dx11.h>
#include <ImGui/inc/imgui_impl_win32.h>

using namespace Engine;
using namespace Engine::Core;
using namespace Engine::Graphics;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

namespace
{
WindowMessageHandler sWindowMessageHandler;

bool IsMouseInput(UINT msg)
{
    switch (msg)
    {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONUP:

    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONUP:

    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
    case WM_MBUTTONUP:

    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
        return true;
    default:
        break;
    }
    return false;
}

bool IsKeyboardMessage(UINT msg)
{
    switch (msg)
    {
    case WM_CHAR:
    case WM_KEYUP:
    case WM_KEYDOWN:
    case WM_SYSKEYUP:
    case WM_SYSKEYDOWN:
        return true;
    default:
        break;
    }
    return false;
}

LRESULT CALLBACK DebugUIMessageHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ImGuiIO& io = ImGui::GetIO();
    // Does ImGui want to capture the mouse & is it a mouse message
    if (io.WantCaptureMouse && IsMouseInput(msg))
    {
        return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
    }

    // Does ImGui want to capture key input & is it a key message
    if (io.WantCaptureMouse && IsKeyboardMessage(msg))
    {
        return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
    }

    LRESULT result = ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
    if (result != 0)
    {
        return result;
    }

    return sWindowMessageHandler.ForwardMessage(hWnd, msg, wParam, lParam);
}
} // namespace

void DebugUI::StaticInitialize(HWND window, bool docking, bool multiViewport)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    if (docking)
    {
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }
    if (multiViewport)
    {
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    }

    auto device = GraphicsSystem::Get()->GetDevice();
    auto context = GraphicsSystem::Get()->GetContext();
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(device, context);

    sWindowMessageHandler.Hook(window, DebugUIMessageHandler);
}

void DebugUI::StaticTerminate()
{
    sWindowMessageHandler.Unhook();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void DebugUI::SetTheme(Theme theme)
{
    switch (theme)
    {
    case Theme::Classic:
    {
        ImGui::StyleColorsClassic();
        break;
    }
    case Theme::Dark:
    {
        ImGui::StyleColorsDark();
        break;
    }
    case Theme::Light:
    {
        ImGui::StyleColorsLight();
        break;
    }
    default:
        ASSERT(false, "DebugUI: Invalid Theme!");
        break;
    }
}

void DebugUI::BeginRender()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void DebugUI::EndRender()
{
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}
