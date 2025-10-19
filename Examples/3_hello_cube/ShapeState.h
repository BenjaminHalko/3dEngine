#pragma once

#include <Engine/Inc/Engine.h>

class ShapeState : public Engine::AppState
{
public:
	void Initialize() override;
	void Terminate() override;
	void Update(float deltaTime) override;
	void Render() override;

protected:
	virtual void CreateShape();

	Engine::Graphics::MeshPC mMesh;
	Engine::Graphics::Camera mCamera;
	Engine::Graphics::ConstantBuffer mTransformBuffer;
	Engine::Graphics::MeshBuffer mMeshbuffer;
	Engine::Graphics::VertexShader mVertexShader;
	Engine::Graphics::PixelShader mPixelShader;
};

class CubeState : public ShapeState
{
public:
	void Update(float deltaTime) override;
protected:
	void CreateShape() override;
};

class PyramidState : public ShapeState
{
public:
	void Update(float deltaTime) override;
protected:
	void CreateShape() override;
};

class RectangleState : public ShapeState
{
public:
	void Update(float deltaTime) override;
protected:
	void CreateShape() override;
};