#include "Precompiled.h"
#include "DebugUI.h"

#include "GraphicsSystem.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_dx11.h>

using namespace Engine;
using namespace Engine::Graphics;
using namespace Engine::Graphics::DebugUI;

namespace
{
Theme sCurrentTheme = Theme::Dark;
} // namespace

void DebugUI::StaticInitialize(GLFWwindow* window, bool docking, bool multiViewport)
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

    SetTheme(sCurrentTheme);

    // Initialize ImGui backends (GLFW + DX11)
    ImGui_ImplGlfw_InitForOther(window, true); // "Other" means we're not using OpenGL

    GraphicsSystem* gs = GraphicsSystem::Get();
    ImGui_ImplDX11_Init(gs->GetDevice(), gs->GetContext());
}

void DebugUI::StaticTerminate()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void DebugUI::SetTheme(Theme theme)
{
    sCurrentTheme = theme;
    switch (theme)
    {
    case Theme::Classic:
        ImGui::StyleColorsClassic();
        break;
    case Theme::Dark:
        ImGui::StyleColorsDark();
        break;
    case Theme::Light:
        ImGui::StyleColorsLight();
        break;
    }
}

void DebugUI::BeginRender()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void DebugUI::EndRender()
{
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Handle multi-viewport if enabled
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}
