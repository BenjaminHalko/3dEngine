#include "Precompiled.h"
#include "InputSystem.h"

using namespace Engine;
using namespace Engine::Input;

namespace
{
std::unique_ptr<InputSystem> sInputSystem;

// GLFW to Engine KeyCode mapping
KeyCode GLFWKeyToKeyCode(int glfwKey)
{
    // Map common GLFW keys to our KeyCode enum
    if (glfwKey >= GLFW_KEY_A && glfwKey <= GLFW_KEY_Z)
        return static_cast<KeyCode>(static_cast<int>(KeyCode::A) + (glfwKey - GLFW_KEY_A));
    if (glfwKey >= GLFW_KEY_0 && glfwKey <= GLFW_KEY_9)
        return static_cast<KeyCode>(static_cast<int>(KeyCode::ZERO) + (glfwKey - GLFW_KEY_0));
    
    switch (glfwKey)
    {
    case GLFW_KEY_SPACE: return KeyCode::SPACE;
    case GLFW_KEY_ESCAPE: return KeyCode::ESCAPE;
    case GLFW_KEY_ENTER: return KeyCode::RETURN;
    case GLFW_KEY_LEFT_SHIFT: return KeyCode::LSHIFT;
    case GLFW_KEY_RIGHT_SHIFT: return KeyCode::RSHIFT;
    case GLFW_KEY_LEFT_CONTROL: return KeyCode::LCONTROL;
    case GLFW_KEY_RIGHT_CONTROL: return KeyCode::RCONTROL;
    case GLFW_KEY_UP: return KeyCode::UP;
    case GLFW_KEY_DOWN: return KeyCode::DOWN;
    case GLFW_KEY_LEFT: return KeyCode::LEFT;
    case GLFW_KEY_RIGHT: return KeyCode::RIGHT;
    default: return static_cast<KeyCode>(glfwKey);
    }
}
} // namespace

void InputSystem::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (sInputSystem && key >= 0 && key < 512)
    {
        KeyCode keyCode = GLFWKeyToKeyCode(key);
        int index = static_cast<int>(keyCode);
        if (index >= 0 && index < 512)
        {
            sInputSystem->mCurrKeys[index] = (action != GLFW_RELEASE);
        }
    }
}

void InputSystem::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (sInputSystem)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            sInputSystem->mCurrMouseButtons[0] = (action == GLFW_PRESS);
        else if (button == GLFW_MOUSE_BUTTON_RIGHT)
            sInputSystem->mCurrMouseButtons[1] = (action == GLFW_PRESS);
        else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            sInputSystem->mCurrMouseButtons[2] = (action == GLFW_PRESS);
    }
}

void InputSystem::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (sInputSystem)
    {
        sInputSystem->mMouseWheel = static_cast<float>(yoffset);
    }
}

void InputSystem::CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (sInputSystem)
    {
        sInputSystem->mCurrMouseX = static_cast<int>(xpos);
        sInputSystem->mCurrMouseY = static_cast<int>(ypos);
    }
}

void InputSystem::StaticInitialize(GLFWwindow* window)
{
    ASSERT(sInputSystem == nullptr, "InputSystem: already initialized!");
    sInputSystem = std::make_unique<InputSystem>();
    sInputSystem->Initialize(window);
}

void InputSystem::StaticTerminate()
{
    if (sInputSystem != nullptr)
    {
        sInputSystem->Terminate();
        sInputSystem.reset();
    }
}

InputSystem* InputSystem::Get()
{
    ASSERT(sInputSystem != nullptr, "InputSystem: is not initialized");
    return sInputSystem.get();
}

InputSystem::~InputSystem()
{
    ASSERT(!mInitialized, "InputSystem: must be terminated first!");
}

void InputSystem::Initialize(GLFWwindow* window)
{
    mWindow = window;

    // Set up GLFW callbacks
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);

    // Initialize mouse position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    mCurrMouseX = mPrevMouseX = static_cast<int>(xpos);
    mCurrMouseY = mPrevMouseY = static_cast<int>(ypos);

    mInitialized = true;
}

void InputSystem::Terminate()
{
    // Clear GLFW callbacks
    if (mWindow)
    {
        glfwSetKeyCallback(mWindow, nullptr);
        glfwSetMouseButtonCallback(mWindow, nullptr);
        glfwSetScrollCallback(mWindow, nullptr);
        glfwSetCursorPosCallback(mWindow, nullptr);
    }

    mWindow = nullptr;
    mInitialized = false;
}

void InputSystem::Update()
{
    // Update previous state
    std::memcpy(mPrevKeys, mCurrKeys, sizeof(mCurrKeys));
    std::memcpy(mPrevMouseButtons, mCurrMouseButtons, sizeof(mCurrMouseButtons));

    // Calculate pressed keys
    for (int i = 0; i < 512; ++i)
    {
        mPressedKeys[i] = !mPrevKeys[i] && mCurrKeys[i];
    }

    // Calculate pressed mouse buttons
    for (int i = 0; i < 3; ++i)
    {
        mPressedMouseButtons[i] = !mPrevMouseButtons[i] && mCurrMouseButtons[i];
    }

    // Calculate mouse movement
    mMouseMoveX = mCurrMouseX - mPrevMouseX;
    mMouseMoveY = mCurrMouseY - mPrevMouseY;
    mPrevMouseX = mCurrMouseX;
    mPrevMouseY = mCurrMouseY;

    // Check edge proximity
    if (mWindow)
    {
        int windowWidth, windowHeight;
        glfwGetWindowSize(mWindow, &windowWidth, &windowHeight);

        const int edgeThreshold = 10;
        mMouseLeftEdge = (mCurrMouseX < edgeThreshold);
        mMouseRightEdge = (mCurrMouseX > windowWidth - edgeThreshold);
        mMouseTopEdge = (mCurrMouseY < edgeThreshold);
        mMouseBottomEdge = (mCurrMouseY > windowHeight - edgeThreshold);
    }

    // Reset mouse wheel after processing
    mMouseWheel = 0.0f;
}

bool InputSystem::IsKeyDown(KeyCode key) const
{
    return mCurrKeys[static_cast<int>(key)];
}

bool InputSystem::IsKeyPressed(KeyCode key) const
{
    return mPressedKeys[static_cast<int>(key)];
}

bool InputSystem::IsMouseDown(MouseButton button) const
{
    return mCurrMouseButtons[static_cast<int>(button)];
}

bool InputSystem::IsMousePressed(MouseButton button) const
{
    return mPressedMouseButtons[static_cast<int>(button)];
}

int InputSystem::GetMouseMoveX() const
{
    return mMouseMoveX;
}

int InputSystem::GetMouseMoveY() const
{
    return mMouseMoveY;
}

float InputSystem::GetMouseMoveZ() const
{
    return mMouseWheel;
}

int InputSystem::GetMouseScreenX() const
{
    return mCurrMouseX;
}

int InputSystem::GetMouseScreenY() const
{
    return mCurrMouseY;
}

bool InputSystem::IsMouseLeftEdge() const
{
    return mMouseLeftEdge;
}

bool InputSystem::IsMouseRightEdge() const
{
    return mMouseRightEdge;
}

bool InputSystem::IsMouseTopEdge() const
{
    return mMouseTopEdge;
}

bool InputSystem::IsMouseBottomEdge() const
{
    return mMouseBottomEdge;
}

void InputSystem::ShowSystemCursor(bool show)
{
    if (mWindow)
    {
        glfwSetInputMode(mWindow, GLFW_CURSOR, show ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
    }
}

void InputSystem::SetMouseClipToWindow(bool clip)
{
    mClipMouseToWindow = clip;
    // GLFW doesn't have direct cursor clipping, but GLFW_CURSOR_DISABLED captures the cursor
    if (mWindow && clip)
    {
        glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else if (mWindow && !clip)
    {
        glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

bool InputSystem::IsMouseClipToWindow() const
{
    return mClipMouseToWindow;
}
