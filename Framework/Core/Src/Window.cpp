#include "Precompiled.h"
#include "Window.h"

using namespace Engine;
using namespace Engine::Core;

namespace
{
    // Convert wstring to UTF-8 string for GLFW
    std::string WStringToString(const std::wstring& wstr)
    {
        if (wstr.empty()) return std::string();
        // Simple conversion for cross-platform use
        std::string result;
        result.reserve(wstr.size());
        for (wchar_t c : wstr) 
        {
            if (c < 128) // ASCII range
                result.push_back(static_cast<char>(c));
            else
                result.push_back('?'); // Replace non-ASCII with placeholder
        }
        return result;
    }
}

void Window::Initialize(void* instance,
                        const std::wstring& appName,
                        uint32_t width,
                        uint32_t height)
{
    mAppName = appName;
    mWidth = width;
    mHeight = height;

    // Initialize GLFW
    if (!glfwInit())
    {
        // Error handling
        return;
    }

    // GLFW window hints for DirectX/DXMT
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // We're using DirectX, not OpenGL
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // Create window
    std::string titleUtf8 = WStringToString(appName);
    mWindow = glfwCreateWindow(width, height, titleUtf8.c_str(), nullptr, nullptr);

    if (mWindow == nullptr)
    {
        glfwTerminate();
        return;
    }

    mIsActive = true;
}

void Window::Terminate()
{
    if (mWindow != nullptr)
    {
        glfwDestroyWindow(mWindow);
        mWindow = nullptr;
    }
    glfwTerminate();
    mIsActive = false;
}

void Window::ProcessMessage()
{
    glfwPollEvents();
    
    // Check if window should close
    if (glfwWindowShouldClose(mWindow))
    {
        mIsActive = false;
    }
}

GLFWwindow* Window::GetWindowHandle() const
{
    return mWindow;
}

bool Window::IsActive() const
{
    return mIsActive;
}
