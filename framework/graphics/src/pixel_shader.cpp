#include "precompiled.h"
#include "pixel_shader.h"

#include "graphics_system.h"

using namespace Engine;
using namespace Engine::Graphics;

void PixelShader::Initialize(const std::filesystem::path& shaderPath)
{
    auto device = GraphicsSystem::Get()->GetDevice();
    DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG;
    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    // BIND TO PIXEL FUNCTION IN SPECIFIED SHADER FILE
    HRESULT hr = D3DCompileFromFile(
        shaderPath.c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "PS", "ps_5_0",
        shaderFlags, 0,
        &shaderBlob,
        &errorBlob);

    if (errorBlob != nullptr && errorBlob->GetBufferPointer() != nullptr)
    {
        LOG("%s", static_cast<const char*>(errorBlob->GetBufferPointer()));
    }
    ASSERT(SUCCEEDED(hr), "Failed to create Pixel Shader");

    hr = device->CreatePixelShader(
        shaderBlob->GetBufferPointer(),
        shaderBlob->GetBufferSize(),
        nullptr,
        &mPixelShader);
    ASSERT(SUCCEEDED(hr), "Failed to create Pixel Shader");
    SafeRelease(shaderBlob);
    SafeRelease(errorBlob);
}

void PixelShader::Terminate()
{
    SafeRelease(mPixelShader);
}

void PixelShader::Bind() const {
    auto context = GraphicsSystem::Get()->GetContext();
    // Bind Buffers
    context->PSSetShader(mPixelShader, nullptr, 0);
}
