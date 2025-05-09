#include "shape_state.h"

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
		Engine::MainApp().ChangeState("Crystal");
	}

	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::RIGHT))
	{
		Engine::MainApp().ChangeState("Heart");
	}
}

void ShapeState::CreateShape()
{
	mVertices.push_back({ { -0.5f,0.0f,0.0f }, Colors::Red });
	mVertices.push_back({ {  0.0f,0.75f,0.0f }, Colors::Blue });
	mVertices.push_back({ {  0.5f,0.0f,0.0f }, Colors::Green });

	mVertices.push_back({ { -0.5f,0.0f,0.0f }, Colors::Red });
	mVertices.push_back({ {  0.5f,0.0f,0.0f }, Colors::Blue });
	mVertices.push_back({ {  0.0f, -0.75f,0.0f }, Colors::Green });
}

void TriForce::Update(float deltaTime)
{
	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::DOWN))
	{
		Engine::MainApp().ChangeState("ShapeState");
	}

	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::LEFT))
	{
		Engine::MainApp().ChangeState("Crystal");
	}

	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::RIGHT))
	{
		Engine::MainApp().ChangeState("Heart");
	}
}

void TriForce::CreateShape()
{
	mVertices.clear();

	float scale = 0.4f;
	float h_offset = 0.0f;
	float v_offset = 0.0f;

	mVertices.push_back({ {h_offset - 0.5f * scale, v_offset + 0.0f * scale, 0.0f}, Colors::Goldenrod });
	mVertices.push_back({ {h_offset + 1.0f * scale, v_offset + 1.0f * scale, 0.0f}, Colors::Goldenrod });
	mVertices.push_back({ {h_offset + 0.0f * scale, v_offset - 1.0f * scale, 0.0f}, Colors::Goldenrod });

	mVertices.push_back({ {h_offset - 0.5f * scale, v_offset + 0.0f * scale, 0.0f}, Colors::DarkGoldenrod });
	mVertices.push_back({ {h_offset + 0.0f * scale, v_offset + 1.0f * scale, 0.0f}, Colors::DarkGoldenrod });
	mVertices.push_back({ {h_offset + 1.0f * scale, v_offset - 1.0f * scale, 0.0f}, Colors::DarkGoldenrod });
}

void Crystal::Update(float deltaTime)
{
	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::DOWN))
	{
		Engine::MainApp().ChangeState("ShapeState");
	}

	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::UP))
	{
		Engine::MainApp().ChangeState("TriForce");
	}

	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::LEFT))
	{
		Engine::MainApp().ChangeState("Crystal");
	}

	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::RIGHT))
	{
		Engine::MainApp().ChangeState("Heart");
	}
}

void Crystal::CreateShape()
{
	mVertices.clear();

	// Define points
	Vector3 p0 = {  0.0f,  0.7f, 0.0f }; // Top
	Vector3 p1 = { -0.2f,  0.4f, 0.0f }; // Upper left
	Vector3 p2 = {  0.2f,  0.4f, 0.0f }; // Upper right
	Vector3 p3 = { -0.3f,  0.0f, 0.0f }; // Middle left
	Vector3 p4 = {  0.3f,  0.0f, 0.0f }; // Middle right
	Vector3 p5 = { -0.2f, -0.4f, 0.0f }; // Lower left
	Vector3 p6 = {  0.2f, -0.4f, 0.0f }; // Lower right
	Vector3 p7 = {  0.0f, -0.7f, 0.0f }; // Bottom

	// Top triangle
	mVertices.push_back({ p0, Colors::White });
	mVertices.push_back({ p1, Colors::Aqua });
	mVertices.push_back({ p2, Colors::Aqua });

	// Upper left facet
	mVertices.push_back({ p0, Colors::White });
	mVertices.push_back({ p3, Colors::Aquamarine });
	mVertices.push_back({ p1, Colors::Aqua });

	// Upper right facet
	mVertices.push_back({ p0, Colors::White });
	mVertices.push_back({ p2, Colors::Aqua });
	mVertices.push_back({ p4, Colors::Aquamarine });

	// Left center
	mVertices.push_back({ p1, Colors::Aqua });
	mVertices.push_back({ p3, Colors::Aquamarine });
	mVertices.push_back({ p5, Colors::Cyan });

	// Right center
	mVertices.push_back({ p2, Colors::Aqua });
	mVertices.push_back({ p6, Colors::Cyan });
	mVertices.push_back({ p4, Colors::Aquamarine });

	// Top band (fills between p1, p2, p3, p4)
	mVertices.push_back({ p1, Colors::Aqua });
	mVertices.push_back({ p4, Colors::Aquamarine });
	mVertices.push_back({ p3, Colors::Aquamarine });

	mVertices.push_back({ p1, Colors::Aqua });
	mVertices.push_back({ p2, Colors::Aqua });
	mVertices.push_back({ p4, Colors::Aquamarine });

	// Center (bottom band, as you already have)
	mVertices.push_back({ p3, Colors::Aquamarine });
	mVertices.push_back({ p4, Colors::Aquamarine });
	mVertices.push_back({ p5, Colors::Cyan });

	mVertices.push_back({ p4, Colors::Aquamarine });
	mVertices.push_back({ p6, Colors::Cyan });
	mVertices.push_back({ p5, Colors::Cyan });

	// Lower left
	mVertices.push_back({ p5, Colors::Cyan });
	mVertices.push_back({ p7, Colors::AliceBlue });
	mVertices.push_back({ p6, Colors::Cyan });

	// Lower right (not strictly needed, but for symmetry)
	mVertices.push_back({ p6, Colors::Cyan });
	mVertices.push_back({ p7, Colors::AliceBlue });
	mVertices.push_back({ p5, Colors::Cyan });
}

void Heart::Update(float deltaTime)
{
	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::DOWN))
	{
		Engine::MainApp().ChangeState("ShapeState");
	}

	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::LEFT))
	{
		Engine::MainApp().ChangeState("Crystal");
	}

	if (Input::InputSystem::Get()->IsKeyPressed(Input::KeyCode::UP))
	{
		Engine::MainApp().ChangeState("TriForce");
	}
}

void Heart::CreateShape()
{
	// Top left lobe
	mVertices.push_back({ { -0.15f, 0.5f, 0.0f }, Colors::DarkRed });
	mVertices.push_back({ { -0.4f, 0.1f, 0.0f }, Colors::PaleVioletRed });
	mVertices.push_back({ { -0.1f, 0.1f, 0.0f }, Colors::PaleVioletRed });

	// Top right lobe
	mVertices.push_back({ { 0.15f, 0.5f, 0.0f }, Colors::DarkRed});
	mVertices.push_back({ { 0.4f, 0.1f, 0.0f }, Colors::PaleVioletRed});
	mVertices.push_back({ { -0.1f, 0.1f, 0.0f }, Colors::PaleVioletRed});

	// Bottom triangle
	mVertices.push_back({ { -0.4f, 0.1f, 0.0f },Colors::PaleVioletRed });
	mVertices.push_back({ { 0.4f, 0.1f, 0.0f }, Colors::PaleVioletRed});
	mVertices.push_back({ { 0.0f, -0.65f, 0.0f },Colors::DarkRed });
}