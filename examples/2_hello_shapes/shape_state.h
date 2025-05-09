#pragma once

#include <engine/inc/engine.h>

class ShapeState : public Engine::AppState
{
public:
    void Initialize() override;
    void Terminate() override;
    void Update(float deltaTime) override;
    void Render() override;

protected:
    virtual void CreateShape();

    struct Vertex
    {
        Engine::Math::Vector3 position;
        Engine::Graphics::Color color;
    };
    using Vertices = std::vector<Vertex>;
    Vertices mVertices;

    ID3D11Buffer* mVertexBuffer = nullptr;
    ID3D11VertexShader* mVertexShader = nullptr;
    ID3D11InputLayout* mInputLayout = nullptr;
    ID3D11PixelShader* mPixelShader = nullptr;
};

class TriForce final : public ShapeState
{
public:
    void Update(float deltaTime) override;
protected:
    void CreateShape() override;
};

class Crystal final : public ShapeState
{
public:
    void Update(float deltaTime) override;
protected:
    void CreateShape() override;
};

class Heart final : public ShapeState
{
public:
    void Update(float deltaTime) override;
protected:
    void CreateShape() override;
};