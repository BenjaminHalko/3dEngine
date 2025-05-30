#pragma once

// External Libraries
#include <Core/Inc\Core.h>
#include <Math/Inc/DWMath.h>

// DirectX 11
#include <d3d11_1.h>
#include <d3dcompiler.h>

#include <DirectXTK/Inc/CommonStates.h>

#include <ImGui/Inc/imgui.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

template<class T>
inline void SafeRelease(T*& ptr)
{
	if (ptr != nullptr)
	{
		ptr->Release();
		ptr = nullptr;
	}
}