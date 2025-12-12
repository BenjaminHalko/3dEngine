// Separate compilation unit for X11 to avoid Window name conflict
#if !defined(_WIN32) && !defined(__APPLE__)

#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

extern "C" void* GetX11WindowHandle(GLFWwindow* window)
{
    return (void*)(uintptr_t)glfwGetX11Window(window);
}

#endif
