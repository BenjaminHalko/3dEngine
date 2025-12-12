#include "Precompiled.h"
#include "VertexShader.h"

#include "GraphicsSystem.h"
#include "VertexTypes.h"

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

std::vector<D3D11_INPUT_ELEMENT_DESC> GetVertexLayout(uint32_t format)
{
    std::vector<D3D11_INPUT_ELEMENT_DESC> vertexLayout;
    if (format & VE_Position)
    {
        vertexLayout.push_back({"POSITION",
                                0,
                                DXGI_FORMAT_R32G32B32_FLOAT,
                                0,
                                D3D11_APPEND_ALIGNED_ELEMENT,
                                D3D11_INPUT_PER_VERTEX_DATA,
                                0});
    }
    if (format & VE_Normal)
    {
        vertexLayout.push_back({"NORMAL",
                                0,
                                DXGI_FORMAT_R32G32B32_FLOAT,
                                0,
                                D3D11_APPEND_ALIGNED_ELEMENT,
                                D3D11_INPUT_PER_VERTEX_DATA,
                                0});
    }
    if (format & VE_Tangent)
    {
        vertexLayout.push_back({"TANGENT",
                                0,
                                DXGI_FORMAT_R32G32B32_FLOAT,
                                0,
                                D3D11_APPEND_ALIGNED_ELEMENT,
                                D3D11_INPUT_PER_VERTEX_DATA,
                                0});
    }
    if (format & VE_Color)
    {
        vertexLayout.push_back({"COLOR",
                                0,
                                DXGI_FORMAT_R32G32B32A32_FLOAT,
                                0,
                                D3D11_APPEND_ALIGNED_ELEMENT,
                                D3D11_INPUT_PER_VERTEX_DATA,
                                0});
    }
    if (format & VE_TexCoord)
    {
        vertexLayout.push_back({"TEXCOORD",
                                0,
                                DXGI_FORMAT_R32G32_FLOAT,
                                0,
                                D3D11_APPEND_ALIGNED_ELEMENT,
                                D3D11_INPUT_PER_VERTEX_DATA,
                                0});
    }

    return vertexLayout;
}
} // namespace

void VertexShader::Initialize(const std::filesystem::path& shaderPath, uint32_t format)
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
                            "VS",
                            "vs_5_0",
                            shaderFlags,
                            0,
                            &shaderBlob,
                            &errorBlob);

    if (errorBlob != nullptr && errorBlob->GetBufferPointer() != nullptr)
    {
        LOG("Vertex Shader Error: %s", static_cast<const char*>(errorBlob->GetBufferPointer()));
    }
    ASSERT(SUCCEEDED(hr), "Failed to create Vertex Shader");

    hr = device->CreateVertexShader(
        shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &mVertexShader);
    ASSERT(SUCCEEDED(hr), "Failed to create Vertex Shader");
    //======================================================================================================

    // STATE WHAT THE VERTEX VARIABLES ARE
    std::vector<D3D11_INPUT_ELEMENT_DESC> vertexLayout = GetVertexLayout(format);

    hr = device->CreateInputLayout(vertexLayout.data(),
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
