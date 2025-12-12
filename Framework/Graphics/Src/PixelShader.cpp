#include "Precompiled.h"
#include "PixelShader.h"

#include "GraphicsSystem.h"

using namespace Engine;
using namespace Engine::Graphics;

namespace
{
    std::string ReadFileContents(const std::filesystem::path& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
            return "";
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
}

void PixelShader::Initialize(const std::filesystem::path& shaderPath)
{
    auto device = GraphicsSystem::Get()->GetDevice();
    DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG;
    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    // Read shader source and compile
    std::string shaderSource = ReadFileContents(shaderPath);
    ASSERT(!shaderSource.empty(), "Failed to read shader file: %s", shaderPath.string().c_str());

    std::string fileName = shaderPath.filename().string();
    HRESULT hr = D3DCompile(shaderSource.c_str(),
                            shaderSource.size(),
                            fileName.c_str(),
                            nullptr,
                            D3D_COMPILE_STANDARD_FILE_INCLUDE,
                            "PS",
                            "ps_5_0",
                            shaderFlags,
                            0,
                            &shaderBlob,
                            &errorBlob);

    if (errorBlob != nullptr && errorBlob->GetBufferPointer() != nullptr)
    {
        LOG("%s", static_cast<const char*>(errorBlob->GetBufferPointer()));
    }
    ASSERT(SUCCEEDED(hr), "Failed to create Pixel Shader");

    hr = device->CreatePixelShader(
        shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &mPixelShader);
    ASSERT(SUCCEEDED(hr), "Failed to create Pixel Shader");
    SafeRelease(shaderBlob);
    SafeRelease(errorBlob);
}

void PixelShader::Terminate()
{
    SafeRelease(mPixelShader);
}

void PixelShader::Bind()
{
    auto context = GraphicsSystem::Get()->GetContext();
    // Bind Buffers
    context->PSSetShader(mPixelShader, nullptr, 0);
}
