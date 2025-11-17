#pragma once

// External Libraries
#include <Core/Inc/Core.h>
#include <Math/Inc/DWMath.h>

// DirectX 11 (native on Windows, DXMT on macOS)
#include <d3d11_1.h>
#include <d3dcompiler.h>

// ImGui
#include <imgui.h>

// Windows-specific pragmas
#ifdef _WIN32
    #pragma comment(lib, "d3d11.lib")
    #pragma comment(lib, "d3dcompiler.lib")
    #pragma comment(lib, "dxguid.lib")
#endif

template <class T> inline void SafeRelease(T*& ptr)
{
    if (ptr != nullptr)
    {
        ptr->Release();
        ptr = nullptr;
    }
}
