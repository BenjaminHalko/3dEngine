#include "ShapeState.h"

using namespace Engine;
using namespace Engine::Math;
using namespace Engine::Graphics;

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
	std::filesystem::path shaderFilePath = L"Assets/Shaders/DoColor.fx";

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
	//======================================================================================================

	// STATE WHAT THE VERTEX VARIABLES ARE
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
	//======================================================================================================

	// BIND TO PIXEL FUNCTION IN SPECIFIED SHADER FILE
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
		Engine::MainApp().ChangeState("TriForce");
	}

	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::LEFT))
	{
		Engine::MainApp().ChangeState("House");
	}

	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::RIGHT))
	{
		Engine::MainApp().ChangeState("Heart");
	}
}

void ShapeState::CreateShape()
{
	mVertices.push_back({ { -0.5f,0.0f,0.0f }, Colors::Red });
	mVertices.push_back({ {  0.0f,0.75f,0.0f }, Colors::Yellow });
	mVertices.push_back({ {  0.5f,0.0f,0.0f }, Colors::Red });

	mVertices.push_back({ { -0.5f,0.0f,0.0f }, Colors::Red });
	mVertices.push_back({ {  0.5f,0.0f,0.0f }, Colors::Yellow });
	mVertices.push_back({ {  0.0f, -0.75f,0.0f }, Colors::Red });
}

void TriForce::Update(float deltaTime)
{
	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::DOWN))
	{
		Engine::MainApp().ChangeState("ShapeState");
	}

	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::LEFT))
	{
		Engine::MainApp().ChangeState("House");
	}

	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::RIGHT))
	{
		Engine::MainApp().ChangeState("Heart");
	}
}

void TriForce::CreateShape()
{
	// Top triangle
	mVertices.push_back({ { -0.25f, 0.25f, 0.0f }, Colors::Yellow });
	mVertices.push_back({ { 0.0f, 0.5f, 0.0f }, Colors::Yellow });
	mVertices.push_back({ { 0.25f, 0.25f, 0.0f }, Colors::Yellow });

	// Bottom-left triangle
	mVertices.push_back({ { -0.5f, 0.0f, 0.0f }, Colors::Yellow });
	mVertices.push_back({ { -0.25f, 0.25f, 0.0f }, Colors::Yellow });
	mVertices.push_back({ { 0.0f, 0.0f, 0.0f }, Colors::Yellow });

	// Bottom-right triangle
	mVertices.push_back({ { 0.5f, 0.0f, 0.0f }, Colors::Yellow });
	mVertices.push_back({ { 0.0f, 0.0f, 0.0f }, Colors::Yellow });
	mVertices.push_back({ { 0.25f, 0.25f, 0.0f }, Colors::Yellow });
}

void House::Update(float deltaTime)
{
	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::DOWN))
	{
		Engine::MainApp().ChangeState("ShapeState");
	}

	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::UP))
	{
		Engine::MainApp().ChangeState("TriForce");
	}

	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::RIGHT))
	{
		Engine::MainApp().ChangeState("Heart");
	}
}

void House::CreateShape()
{
	// Roof
	mVertices.push_back({ { -0.5f, 0.2f, 0.0f }, Colors::DarkRed });
	mVertices.push_back({ { 0.0f, 0.55f, 0.0f }, Colors::DarkRed });
	mVertices.push_back({ { 0.5f, 0.2f, 0.0f }, Colors::DarkRed });

	// Base of the house
	mVertices.push_back({ { -0.4f, 0.2f, 0.0f }, Colors::AntiqueWhite });
	mVertices.push_back({ { 0.4f, 0.2f, 0.0f }, Colors::AntiqueWhite });
	mVertices.push_back({ { 0.4f, -0.3f, 0.0f }, Colors::AntiqueWhite });

	mVertices.push_back({ { -0.4f, 0.2f, 0.0f }, Colors::AntiqueWhite });
	mVertices.push_back({ { 0.4f, -0.3f, 0.0f }, Colors::AntiqueWhite });
	mVertices.push_back({ { -0.4f, -0.3f, 0.0f }, Colors::AntiqueWhite });

	// Door
	mVertices.push_back({ { -0.1f, 0.0f, 0.0f }, Colors::SaddleBrown });
	mVertices.push_back({ { -0.1f, -0.3f, 0.0f }, Colors::SaddleBrown });
	mVertices.push_back({ { 0.1f, -0.3f, 0.0f }, Colors::SaddleBrown });

	mVertices.push_back({ { -0.1f, 0.0f, 0.0f }, Colors::SaddleBrown });
	mVertices.push_back({ { 0.1f, 0.0f, 0.0f }, Colors::SaddleBrown });
	mVertices.push_back({ { 0.1f, -0.3f, 0.0f }, Colors::SaddleBrown });
}

void Heart::Update(float deltaTime)
{
	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::DOWN))
	{
		Engine::MainApp().ChangeState("ShapeState");
	}

	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::LEFT))
	{
		Engine::MainApp().ChangeState("House");
	}

	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::UP))
	{
		Engine::MainApp().ChangeState("TriForce");
	}
}

void Heart::CreateShape()
{
	// Top left lobe
	mVertices.push_back({ { -0.4f, 0.1f, 0.0f }, Colors::PaleVioletRed });
	mVertices.push_back({ { -0.15f, 0.5f, 0.0f }, Colors::DarkRed });
	mVertices.push_back({ { 0.1f, 0.1f, 0.0f }, Colors::PaleVioletRed });

	// Top right lobe
	mVertices.push_back({ { 0.15f, 0.5f, 0.0f }, Colors::DarkRed});
	mVertices.push_back({ { 0.4f, 0.1f, 0.0f }, Colors::PaleVioletRed});
	mVertices.push_back({ { -0.1f, 0.1f, 0.0f }, Colors::PaleVioletRed});

	// Bottom triangle
	mVertices.push_back({ { -0.4f, 0.1f, 0.0f },Colors::PaleVioletRed });
	mVertices.push_back({ { 0.4f, 0.1f, 0.0f }, Colors::PaleVioletRed});
	mVertices.push_back({ { 0.0f, -0.65f, 0.0f },Colors::DarkRed });
}