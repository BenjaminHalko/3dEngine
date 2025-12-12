#include "ShapeState.h"

using namespace Engine;
using namespace Engine::Math;
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

void ShapeState::Initialize()
{
    // Creates a shape out of the vertices
    CreateShape();

    auto device = GraphicsSystem::Get()->GetDevice();

    // Need to create a buffer to store the vertices
    // STORES DATA FOR THE OBJECT
    D3D11_BUFFER_DESC bufferDesc{};
    bufferDesc.ByteWidth = static_cast<UINT>(mVertices.size()) * sizeof(Vertex);
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = mVertices.data();

    HRESULT hr = device->CreateBuffer(&bufferDesc, &initData, &mVertexBuffer);
    ASSERT(SUCCEEDED(hr), "Failed to create Vertex Buffer");
    //====================================================================================================

    // BIND TO FUNCTION IN SPECIFIED SHADER FILE
    std::filesystem::path shaderFilePath = "Assets/Shaders/DoColor.fx";

    std::string shaderSource = ReadFileContents(shaderFilePath);
    ASSERT(!shaderSource.empty(), "Failed to read shader file");

    DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG;
    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    std::string fileName = shaderFilePath.filename().string();
    hr = D3DCompile(shaderSource.c_str(),
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
        LOG("%s", static_cast<const char*>(errorBlob->GetBufferPointer()));
    }
    ASSERT(SUCCEEDED(hr), "Failed to create Vertex Shader");

    hr = device->CreateVertexShader(
        shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &mVertexShader);
    ASSERT(SUCCEEDED(hr), "Failed to create Vertex Shader");
    //======================================================================================================

    // STATE WHAT THE VERTEX VARIABLES ARE
    std::vector<D3D11_INPUT_ELEMENT_DESC> vertexLayout;
    vertexLayout.push_back(
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT});
    vertexLayout.push_back(
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT});

    hr = device->CreateInputLayout(vertexLayout.data(),
                                   static_cast<UINT>(vertexLayout.size()),
                                   shaderBlob->GetBufferPointer(),
                                   shaderBlob->GetBufferSize(),
                                   &mInputLayout);
    ASSERT(SUCCEEDED(hr), "Failed to create Input Layout");
    SafeRelease(shaderBlob);
    SafeRelease(errorBlob);
    //======================================================================================================

    // BIND TO PIXEL FUNCTION IN SPECIFIED SHADER FILE
    hr = D3DCompile(shaderSource.c_str(),
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
    // Bind buffers
    context->VSSetShader(mVertexShader, nullptr, 0);
    context->IASetInputLayout(mInputLayout);
    context->PSSetShader(mPixelShader, nullptr, 0);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Draw
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &offset);
    context->Draw(static_cast<UINT>(mVertices.size()), 0);
}

void ShapeState::Update(float deltaTime)
{
    if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::UP))
    {
        MainApp().ChangeState("TriForce");
    }

    if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::LEFT))
    {
        MainApp().ChangeState("House");
    }

    if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::RIGHT))
    {
        MainApp().ChangeState("Heart");
    }
}

void ShapeState::CreateShape()
{
    mVertices.push_back({{-0.5f, 0.0f, 0.0f}, Colors::Red});
    mVertices.push_back({{0.0f, 0.75f, 0.0f}, Colors::Yellow});
    mVertices.push_back({{0.5f, 0.0f, 0.0f}, Colors::Red});
}

void TriForce::CreateShape()
{
    // Top Triangle
    mVertices.push_back({{-0.25f, 0.5f, 0.0f}, Colors::Red});
    mVertices.push_back({{0.0f, 0.75f, 0.0f}, Colors::Yellow});
    mVertices.push_back({{0.25f, 0.5f, 0.0f}, Colors::Red});

    // Bottom Left Triangle
    mVertices.push_back({{-0.5f, 0.0f, 0.0f}, Colors::Green});
    mVertices.push_back({{-0.25f, 0.5f, 0.0f}, Colors::Blue});
    mVertices.push_back({{0.0f, 0.0f, 0.0f}, Colors::Green});

    // Bottom Right Triangle
    mVertices.push_back({{0.0f, 0.0f, 0.0f}, Colors::Magenta});
    mVertices.push_back({{0.25f, 0.5f, 0.0f}, Colors::Cyan});
    mVertices.push_back({{0.5f, 0.0f, 0.0f}, Colors::Magenta});
}

void TriForce::Update(float deltaTime)
{
    ShapeState::Update(deltaTime);
}

void House::CreateShape()
{
    // Base of the house (Square)
    mVertices.push_back({{-0.5f, -0.5f, 0.0f}, Colors::Brown});
    mVertices.push_back({{-0.5f, 0.0f, 0.0f}, Colors::Brown});
    mVertices.push_back({{0.5f, -0.5f, 0.0f}, Colors::Brown});

    mVertices.push_back({{0.5f, -0.5f, 0.0f}, Colors::Brown});
    mVertices.push_back({{-0.5f, 0.0f, 0.0f}, Colors::Brown});
    mVertices.push_back({{0.5f, 0.0f, 0.0f}, Colors::Brown});

    // Roof (Triangle)
    mVertices.push_back({{-0.5f, 0.0f, 0.0f}, Colors::Red});
    mVertices.push_back({{0.0f, 0.5f, 0.0f}, Colors::Red});
    mVertices.push_back({{0.5f, 0.0f, 0.0f}, Colors::Red});

    // Door (Rectangle)
    mVertices.push_back({{-0.1f, -0.5f, 0.0f}, Colors::Blue});
    mVertices.push_back({{-0.1f, -0.1f, 0.0f}, Colors::Blue});
    mVertices.push_back({{0.1f, -0.5f, 0.0f}, Colors::Blue});

    mVertices.push_back({{0.1f, -0.5f, 0.0f}, Colors::Blue});
    mVertices.push_back({{-0.1f, -0.1f, 0.0f}, Colors::Blue});
    mVertices.push_back({{0.1f, -0.1f, 0.0f}, Colors::Blue});

    // Window (Square)
    mVertices.push_back({{0.2f, -0.1f, 0.0f}, Colors::LightBlue});
    mVertices.push_back({{0.2f, 0.1f, 0.0f}, Colors::LightBlue});
    mVertices.push_back({{0.4f, -0.1f, 0.0f}, Colors::LightBlue});

    mVertices.push_back({{0.4f, -0.1f, 0.0f}, Colors::LightBlue});
    mVertices.push_back({{0.2f, 0.1f, 0.0f}, Colors::LightBlue});
    mVertices.push_back({{0.4f, 0.1f, 0.0f}, Colors::LightBlue});
}

void House::Update(float deltaTime)
{
    ShapeState::Update(deltaTime);
}

void Heart::CreateShape()
{
    // Heart shape using triangles
    const int numSegments = 20;
    const float radius = 0.15f;
    const Vector3 center1(-0.15f, 0.15f, 0.0f);
    const Vector3 center2(0.15f, 0.15f, 0.0f);
    const Vector3 bottom(0.0f, -0.3f, 0.0f);

    // Left semicircle
    for (int i = 0; i < numSegments / 2; ++i)
    {
        float angle1 = Math::Constants::Pi + (i * Math::Constants::Pi) / (numSegments / 2);
        float angle2 = Math::Constants::Pi + ((i + 1) * Math::Constants::Pi) / (numSegments / 2);

        Vector3 p1 = center1 + Vector3(cos(angle1) * radius, sin(angle1) * radius, 0.0f);
        Vector3 p2 = center1 + Vector3(cos(angle2) * radius, sin(angle2) * radius, 0.0f);

        mVertices.push_back({center1, Colors::Pink});
        mVertices.push_back({p1, Colors::Red});
        mVertices.push_back({p2, Colors::Red});
    }

    // Right semicircle
    for (int i = 0; i < numSegments / 2; ++i)
    {
        float angle1 = (i * Math::Constants::Pi) / (numSegments / 2);
        float angle2 = ((i + 1) * Math::Constants::Pi) / (numSegments / 2);

        Vector3 p1 = center2 + Vector3(cos(angle1) * radius, sin(angle1) * radius, 0.0f);
        Vector3 p2 = center2 + Vector3(cos(angle2) * radius, sin(angle2) * radius, 0.0f);

        mVertices.push_back({center2, Colors::Pink});
        mVertices.push_back({p1, Colors::Red});
        mVertices.push_back({p2, Colors::Red});
    }

    // Bottom triangle connecting the circles
    mVertices.push_back({{-0.15f, 0.0f, 0.0f}, Colors::Red});
    mVertices.push_back({{0.15f, 0.0f, 0.0f}, Colors::Red});
    mVertices.push_back({bottom, Colors::Pink});
}

void Heart::Update(float deltaTime)
{
    ShapeState::Update(deltaTime);
}
