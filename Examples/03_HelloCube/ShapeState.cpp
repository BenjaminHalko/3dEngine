#include "ShapeState.h"

using namespace Engine;
using namespace Engine::Math;
using namespace Engine::Graphics;
using namespace Engine::Input;

void ShapeState::Initialize()
{
    mCamera.SetPosition({0.0f, 1.0f, -3.0f});
    mCamera.SetLookAt({0.0f, 0.0f, 0.0f});

    mTransformBuffer.Initialize(sizeof(Math::Matrix4));

    // Creates a shape out of the vertices
    CreateShape();
    mMeshbuffer.Initialize(mMesh);

    std::filesystem::path shaderFilePath = L"Assets/Shaders/DoTransformColor.fx";
    mVertexShader.Initialize<VertexPC>(shaderFilePath);
    mPixelShader.Initialize(shaderFilePath);
}

void ShapeState::Terminate()
{
    mTransformBuffer.Terminate();
    mPixelShader.Terminate();
    mVertexShader.Terminate();
    mMeshbuffer.Terminate();
}

void ShapeState::Update(float deltaTime)
{
    // Camera Controls:
    InputSystem* input = InputSystem::Get();
    const float moveSpeed = input->IsKeyDown(KeyCode::LSHIFT) ? 10.0f : 4.0f;
    const float turnSpeed = 0.5f;

    if (input->IsKeyDown(KeyCode::W))
    {
        mCamera.Walk(moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::S))
    {
        mCamera.Walk(-moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::D))
    {
        mCamera.Strafe(moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::A))
    {
        mCamera.Strafe(-moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::E))
    {
        mCamera.Rise(moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::Q))
    {
        mCamera.Rise(-moveSpeed * deltaTime);
    }

    if (input->IsMouseDown(MouseButton::RBUTTON))
    {
        mCamera.Yaw(input->GetMouseMoveX() * turnSpeed * deltaTime);
        mCamera.Pitch(input->GetMouseMoveY() * turnSpeed * deltaTime);
    }
}

void ShapeState::Render()
{
    mVertexShader.Bind();
    mPixelShader.Bind();

    Math::Matrix4 matWorld = Math::Matrix4::RotationY(TimeUtil::GetTime());
    Math::Matrix4 matView = mCamera.GetViewMatrix();
    Math::Matrix4 matProj = mCamera.GetProjectionMatrix();
    Math::Matrix4 matFinal = matWorld * matView * matProj;
    Math::Matrix4 wvp = Math::Transpose(matFinal);

    mTransformBuffer.Update(&wvp);
    mTransformBuffer.BindVS(0);

    mMeshbuffer.Render();
}

void ShapeState::CreateShape()
{
    // mMesh = MeshBuilder::CreateRectanglePC(1.5f, 1.5f, 2.0f);
    // mMesh = MeshBuilder::CreateCylinderPC(25.0f, 5.0f);
    // mMesh = MeshBuilder::CreatePlanePC(10.f, 10.f, 1);
    // mMesh = MeshBuilder::CreatePyramidPC(5.0f);
    // mMesh = MeshBuilder::CreatePlanePC(10.f, 10.f, 1);
    // mMesh = MeshBuilder::CreateCylinderPC(25.0f, 5.0f);
    mMesh = MeshBuilder::CreateSpherePC(30, 30, 1.0f);
}

void CubeState::Update(float deltaTime)
{
    //====================================================================================================================================================================
    // Camera Controls:
    InputSystem* input = InputSystem::Get();
    const float moveSpeed = input->IsKeyDown(KeyCode::LSHIFT) ? 10.0f : 4.0f;
    const float turnSpeed = 0.5f;

    if (input->IsKeyDown(KeyCode::W))
    {
        mCamera.Walk(moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::S))
    {
        mCamera.Walk(-moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::D))
    {
        mCamera.Strafe(moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::A))
    {
        mCamera.Strafe(-moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::E))
    {
        mCamera.Rise(moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::Q))
    {
        mCamera.Rise(-moveSpeed * deltaTime);
    }

    if (input->IsMouseDown(MouseButton::RBUTTON))
    {
        mCamera.Yaw(input->GetMouseMoveX() * turnSpeed * deltaTime);
        mCamera.Pitch(input->GetMouseMoveY() * turnSpeed * deltaTime);
    }
    //===========================================================================================================================================================================================

    if (InputSystem::Get()->IsKeyPressed(KeyCode::UP))
    {
        MainApp().ChangeState("Pyramid");
    }

    if (InputSystem::Get()->IsKeyPressed(KeyCode::DOWN))
    {
        MainApp().ChangeState("ShapeState");
    }

    if (InputSystem::Get()->IsKeyPressed(KeyCode::RIGHT))
    {
        MainApp().ChangeState("Rectangle");
    }
}
void CubeState::CreateShape()
{
    mMesh = MeshBuilder::CreateCubePC(1.0f);
}

void PyramidState::Update(float deltaTime)
{
    //===============================================================================================================================================
    // Camera Controls:
    InputSystem* input = InputSystem::Get();
    const float moveSpeed = input->IsKeyDown(KeyCode::LSHIFT) ? 10.0f : 4.0f;
    const float turnSpeed = 0.5f;

    if (input->IsKeyDown(KeyCode::W))
    {
        mCamera.Walk(moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::S))
    {
        mCamera.Walk(-moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::D))
    {
        mCamera.Strafe(moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::A))
    {
        mCamera.Strafe(-moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::E))
    {
        mCamera.Rise(moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::Q))
    {
        mCamera.Rise(-moveSpeed * deltaTime);
    }

    if (input->IsMouseDown(MouseButton::RBUTTON))
    {
        mCamera.Yaw(input->GetMouseMoveX() * turnSpeed * deltaTime);
        mCamera.Pitch(input->GetMouseMoveY() * turnSpeed * deltaTime);
    }
    //===========================================================================================================================================================================================

    if (InputSystem::Get()->IsKeyPressed(KeyCode::DOWN))
    {
        MainApp().ChangeState("ShapeState");
    }

    if (InputSystem::Get()->IsKeyPressed(KeyCode::LEFT))
    {
        MainApp().ChangeState("Cube");
    }

    if (InputSystem::Get()->IsKeyPressed(KeyCode::RIGHT))
    {
        MainApp().ChangeState("Rectangle");
    }
}
void PyramidState::CreateShape()
{
    mMesh = MeshBuilder::CreatePyramidPC(1.0f);
}

void RectangleState::Update(float deltaTime)
{
    //===========================================================================================================================================================================================
    // Camera Controls:
    InputSystem* input = InputSystem::Get();
    const float moveSpeed = input->IsKeyDown(KeyCode::LSHIFT) ? 10.0f : 4.0f;
    const float turnSpeed = 0.5f;

    if (input->IsKeyDown(KeyCode::W))
    {
        mCamera.Walk(moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::S))
    {
        mCamera.Walk(-moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::D))
    {
        mCamera.Strafe(moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::A))
    {
        mCamera.Strafe(-moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::E))
    {
        mCamera.Rise(moveSpeed * deltaTime);
    }

    else if (input->IsKeyDown(KeyCode::Q))
    {
        mCamera.Rise(-moveSpeed * deltaTime);
    }

    if (input->IsMouseDown(MouseButton::RBUTTON))
    {
        mCamera.Yaw(input->GetMouseMoveX() * turnSpeed * deltaTime);
        mCamera.Pitch(input->GetMouseMoveY() * turnSpeed * deltaTime);
    }
    //===========================================================================================================================================================================================

    if (InputSystem::Get()->IsKeyPressed(KeyCode::UP))
    {
        MainApp().ChangeState("Pyramid");
    }

    if (InputSystem::Get()->IsKeyPressed(KeyCode::LEFT))
    {
        MainApp().ChangeState("Cube");
    }

    if (InputSystem::Get()->IsKeyPressed(KeyCode::DOWN))
    {
        MainApp().ChangeState("ShapeState");
    }
}
void RectangleState::CreateShape()
{
    mMesh = MeshBuilder::CreateRectanglePC(1.0f, 1.0f, 2.0f);
}
