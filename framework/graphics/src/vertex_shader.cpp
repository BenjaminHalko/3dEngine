#include "precompiled.h"
#include "vertex_shader.h"

#include "graphics_system.h"
#include "vertex_types.h"

using namespace Engine;
using namespace Engine::Graphics;

namespace
{
	std::vector<D3D11_INPUT_ELEMENT_DESC> GetVertexLayout(uint32_t format)
	{
		std::vector<D3D11_INPUT_ELEMENT_DESC> vertexLayout;
		if (format & VE_Position)
		{
			vertexLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });
		}
		if (format & VE_Normal)
		{
			vertexLayout.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });
		}
		if (format & VE_Tangent)
		{
			vertexLayout.push_back({ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });
		}
		if (format & VE_Color)
		{
			vertexLayout.push_back({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });
		}
		if (format & VE_TexCoord)
		{
			vertexLayout.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });
		}

		return vertexLayout;
	}
}

void VertexShader::Initialize(const std::filesystem::path& shaderPath, uint32_t format)
{
	auto device = GraphicsSystem::Get()->GetDevice();

	DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG;
	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	HRESULT hr = D3DCompileFromFile(
		shaderPath.c_str(),
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"VS", "vs_5_0",
		shaderFlags, 0,
		&shaderBlob,
		&errorBlob);

	if (errorBlob != nullptr && errorBlob->GetBufferPointer() != nullptr)
	{
		LOG("%s", static_cast<const char*>(errorBlob->GetBufferPointer()));
	}
	ASSERT(SUCCEEDED(hr), "Failed to create Vertex Shader");

	hr = device->CreateVertexShader(
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		nullptr,
		&mVertexShader);
	ASSERT(SUCCEEDED(hr), "Failed to create Vertex Shader");
	//======================================================================================================

	// STATE WHAT THE VERTEX VARIABLES ARE
	std::vector<D3D11_INPUT_ELEMENT_DESC> vertexLayout = GetVertexLayout(format);

	hr = device->CreateInputLayout(
		vertexLayout.data(),
		static_cast<UINT>(vertexLayout.size()),
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		&mInputLayout);
	ASSERT(SUCCEEDED(hr), "Failed to create Input Layout");
	SafeRelease(shaderBlob);
	SafeRelease(errorBlob);
}

void VertexShader::Terminate()
{
	SafeRelease(mInputLayout);
	SafeRelease(mVertexShader);
}

void VertexShader::Bind()
{
	auto context = GraphicsSystem::Get()->GetContext();
	// Bind buffers
	context->VSSetShader(mVertexShader, nullptr, 0);
	context->IASetInputLayout(mInputLayout);
}
