#include "shape_state.h"

using namespace Engine;
using namespace Engine::Math;
using namespace Engine::Graphics;

using VertexPC = Engine::Graphics::VertexPC;

void ShapeState::Initialize()
{
    // Creates a shape out of the vertices
    CreateShape();

    auto device = GraphicsSystem::Get()->GetDevice();

    // Need to create a buffer to store the vertices
    D3D11_BUFFER_DESC bufferDesc{};
    bufferDesc.ByteWidth = static_cast<UINT>(mVertices.size()) * sizeof(VertexPC);
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = mVertices.data();

    HRESULT hr = device->CreateBuffer(&bufferDesc, &initData, &mVertexBuffer);
    ASSERT(SUCCEEDED(hr), "Failed to create Vertex Buffer");

    std::filesystem::path shaderFilePath = L"assets/shaders/do_color.fx";

    DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG;
    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    hr = D3DCompileFromFile(
        shaderFilePath.c_str(),
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

    std::vector<D3D11_INPUT_ELEMENT_DESC> vertexLayout;
    vertexLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,0,D3D11_APPEND_ALIGNED_ELEMENT });
    vertexLayout.push_back({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D11_APPEND_ALIGNED_ELEMENT });

    hr = device->CreateInputLayout(
        vertexLayout.data(),
        static_cast<UINT>(vertexLayout.size()),
        shaderBlob->GetBufferPointer(),
        shaderBlob->GetBufferSize(),
        &mInputLayout);
    ASSERT(SUCCEEDED(hr), "Failed to create Input Layout");
    SafeRelease(shaderBlob);
    SafeRelease(errorBlob);

    hr = D3DCompileFromFile(
        shaderFilePath.c_str(),
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

void ShapeState::Terminate()
{
    mVertices.clear();
    SafeRelease(mPixelShader);
    SafeRelease(mInputLayout);
    SafeRelease(mVertexShader);
    SafeRelease(mVertexBuffer);
}

void ShapeState::Render()
{
    auto context = GraphicsSystem::Get()->GetContext();
    context->VSSetShader(mVertexShader, nullptr, 0);
    context->IASetInputLayout(mInputLayout);
    context->PSSetShader(mPixelShader, nullptr, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    UINT stride = sizeof(VertexPC);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &offset);
    context->Draw(static_cast<UINT>(mVertices.size()), 0);
}

void ShapeState::Update(float deltaTime)
{
    // No state switching for the cube example
}

void ShapeState::CreateShape()
{
    // Cube vertices (12 triangles, 36 vertices, colored faces)
    // Front face (red)
    mVertices.push_back({ { -0.5f, -0.5f, -0.5f }, Colors::Red });
    mVertices.push_back({ { -0.5f,  0.5f, -0.5f }, Colors::Red });
    mVertices.push_back({ {  0.5f,  0.5f, -0.5f }, Colors::Red });
    mVertices.push_back({ { -0.5f, -0.5f, -0.5f }, Colors::Red });
    mVertices.push_back({ {  0.5f,  0.5f, -0.5f }, Colors::Red });
    mVertices.push_back({ {  0.5f, -0.5f, -0.5f }, Colors::Red });
    // Back face (green)
    mVertices.push_back({ { -0.5f, -0.5f, 0.5f }, Colors::Green });
    mVertices.push_back({ {  0.5f,  0.5f, 0.5f }, Colors::Green });
    mVertices.push_back({ { -0.5f,  0.5f, 0.5f }, Colors::Green });
    mVertices.push_back({ { -0.5f, -0.5f, 0.5f }, Colors::Green });
    mVertices.push_back({ {  0.5f, -0.5f, 0.5f }, Colors::Green });
    mVertices.push_back({ {  0.5f,  0.5f, 0.5f }, Colors::Green });
    // Left face (blue)
    mVertices.push_back({ { -0.5f, -0.5f,  0.5f }, Colors::Blue });
    mVertices.push_back({ { -0.5f,  0.5f,  0.5f }, Colors::Blue });
    mVertices.push_back({ { -0.5f,  0.5f, -0.5f }, Colors::Blue });
    mVertices.push_back({ { -0.5f, -0.5f,  0.5f }, Colors::Blue });
    mVertices.push_back({ { -0.5f,  0.5f, -0.5f }, Colors::Blue });
    mVertices.push_back({ { -0.5f, -0.5f, -0.5f }, Colors::Blue });
    // Right face (yellow)
    mVertices.push_back({ { 0.5f, -0.5f, -0.5f }, Colors::Yellow });
    mVertices.push_back({ { 0.5f,  0.5f, -0.5f }, Colors::Yellow });
    mVertices.push_back({ { 0.5f,  0.5f,  0.5f }, Colors::Yellow });
    mVertices.push_back({ { 0.5f, -0.5f, -0.5f }, Colors::Yellow });
    mVertices.push_back({ { 0.5f,  0.5f,  0.5f }, Colors::Yellow });
    mVertices.push_back({ { 0.5f, -0.5f,  0.5f }, Colors::Yellow });
    // Top face (cyan)
    mVertices.push_back({ { -0.5f, 0.5f, -0.5f }, Colors::Cyan });
    mVertices.push_back({ { -0.5f, 0.5f,  0.5f }, Colors::Cyan });
    mVertices.push_back({ {  0.5f, 0.5f,  0.5f }, Colors::Cyan });
    mVertices.push_back({ { -0.5f, 0.5f, -0.5f }, Colors::Cyan });
    mVertices.push_back({ {  0.5f, 0.5f,  0.5f }, Colors::Cyan });
    mVertices.push_back({ {  0.5f, 0.5f, -0.5f }, Colors::Cyan });
    // Bottom face (magenta)
    mVertices.push_back({ { -0.5f, -0.5f, -0.5f }, Colors::Magenta });
    mVertices.push_back({ {  0.5f, -0.5f,  0.5f }, Colors::Magenta });
    mVertices.push_back({ { -0.5f, -0.5f,  0.5f }, Colors::Magenta });
    mVertices.push_back({ { -0.5f, -0.5f, -0.5f }, Colors::Magenta });
    mVertices.push_back({ {  0.5f, -0.5f, -0.5f }, Colors::Magenta });
    mVertices.push_back({ {  0.5f, -0.5f,  0.5f }, Colors::Magenta });
} 