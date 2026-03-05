#include "GameState.h"

using namespace Engine;
using namespace Engine::Graphics;
using namespace Engine::Input;
using namespace Engine::Physics;

void GameState::Initialize()
{
    mCamera.SetPosition({2.0f, 2.0f, -2.0f});
    mCamera.SetLookAt({0.0f, 1.2f, 0.0f});

    mDirectionalLight.direction = Math::Normalize({1.0f, -1.0f, 1.0f});
    mDirectionalLight.ambient = {0.4f, 0.4f, 0.4f, 1.0f};
    mDirectionalLight.diffuse = {0.8f, 0.8f, 0.8f, 1.0f};
    mDirectionalLight.specular = {0.9f, 0.9f, 0.9f, 1.0f};

    Mesh football = MeshBuilder::CreateSphere(50, 50, 0.5f);
    mFootball.meshBuffer.Initialize(football);
    mFootball.transform.position.y = 5.0f;
    mBallShape.InitializeSphere(0.5f);
    mBallRigidBody.Initialize(mFootball.transform, mBallShape, 5.0f);

    TextureManager* tm = TextureManager::Get();
    mFootball.diffuseMapId = tm->LoadTexture("misc/Brazuca.jpg");

    Mesh plane = MeshBuilder::CreatePlane(20, 20, 1.0f, true);
    mGroundObject.meshBuffer.Initialize(plane);
    mGroundShape.InitializeHull({10.0f, 1.0f, 10.0f}, {0.0f, -0.5f, 0.0f});
    mGroundRigidBody.Initialize(mGroundObject.transform, mGroundShape, 0.0f);

    mGroundObject.diffuseMapId = tm->LoadTexture("misc/concrete.jpg");

    std::filesystem::path shaderFile = "Assets/Shaders/Standard.hlsl";
    mStandardEffect.Initialize(shaderFile);
    mStandardEffect.SetCamera(mCamera);
    mStandardEffect.SetDirectionalLight(mDirectionalLight);

    Mesh boxMesh = MeshBuilder::CreateCube(1.0f);
    TextureId boxTextureId = tm->LoadTexture("misc/cardboard.jpg");

    float yOffset = 4.5f;
    float xOffset = 0.0f;
    int rowCount = 1;
    int boxIndex = 0;
    mBoxes.resize(10);
    while (boxIndex < static_cast<int>(mBoxes.size()))
    {
        xOffset = -((static_cast<float>(rowCount) - 1.0f) * 0.5f);
        for (int r = 0; r < rowCount; ++r)
        {
            BoxData& box = mBoxes[boxIndex];
            box.box.meshBuffer.Initialize(boxMesh);
            box.box.diffuseMapId = boxTextureId;
            box.box.transform.position.x = xOffset;
            box.box.transform.position.y = yOffset;
            box.box.transform.position.z = 4.0f;
            box.shape.InitializeBox({0.5f, 0.5f, 0.5f});
            xOffset += 1.0f;
            ++boxIndex;
        }
        yOffset -= 1.0f;
        rowCount += 1;
    }
    for (int i = static_cast<int>(mBoxes.size()) - 1; i >= 0; --i)
    {
        mBoxes[i].rigidBody.Initialize(mBoxes[i].box.transform, mBoxes[i].shape, 1.0f);
    }

    int rows = 20;
    int cols = 20;
    mClothMesh = MeshBuilder::CreatePlane(rows, cols, 0.5f);
    for (Vertex& v : mClothMesh.vertices)
    {
        v.position.y += 10.0f;
        v.position.z += 10.0f;
    }

    uint32_t lastVertex = static_cast<uint32_t>(mClothMesh.vertices.size()) - 1;
    uint32_t lastVertexOS = lastVertex - cols;
    mClothSoftBody.Initialize(mClothMesh, 1.0f, {lastVertex, lastVertexOS});

    mClothRenderObject.meshBuffer.Initialize(nullptr,
                                             static_cast<uint32_t>(sizeof(Vertex)),
                                             static_cast<uint32_t>(mClothMesh.vertices.size()),
                                             mClothMesh.indices.data(),
                                             static_cast<uint32_t>(mClothMesh.indices.size()));
    mClothRenderObject.diffuseMapId = tm->LoadTexture("misc/cloth.jpg");
}

void GameState::Terminate()
{
    mClothRenderObject.Terminate();
    mClothSoftBody.Terminate();

    for (BoxData& box : mBoxes)
    {
        box.rigidBody.Terminate();
        box.shape.Terminate();
        box.box.Terminate();
    }

    mStandardEffect.Terminate();

    mGroundRigidBody.Terminate();
    mGroundShape.Terminate();
    mGroundObject.Terminate();

    mBallRigidBody.Terminate();
    mBallShape.Terminate();
    mFootball.Terminate();
}

void GameState::Update(float deltaTime)
{
    UpdateCamera(deltaTime);

    if (InputSystem::Get()->IsKeyPressed(KeyCode::SPACE))
    {
        Math::Vector3 spawnPos = mCamera.GetPosition() + (mCamera.GetDirection() * 0.5f);
        mBallRigidBody.SetPosition(spawnPos);
        mBallRigidBody.SetVelocity(mCamera.GetDirection() * 50.0f);
    }
}

void GameState::Render()
{
    mClothRenderObject.meshBuffer.Update(mClothMesh.vertices.data(),
                                         static_cast<uint32_t>(mClothMesh.vertices.size()));

    SimpleDraw::AddGroundPlane(20.0f, Colors::Wheat);
    SimpleDraw::Render(mCamera);

    mStandardEffect.Begin();
    mStandardEffect.Render(mFootball);
    mStandardEffect.Render(mGroundObject);
    mStandardEffect.Render(mClothRenderObject);
    for (BoxData& box : mBoxes)
    {
        mStandardEffect.Render(box.box);
    }
    mStandardEffect.End();
}

void GameState::DebugUI()
{
    ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::DragFloat3("Direction#Light", &mDirectionalLight.direction.x, 0.01f))
        {
            mDirectionalLight.direction = Math::Normalize(mDirectionalLight.direction);
        }

        ImGui::ColorEdit4("Ambient#Light", &mDirectionalLight.ambient.x);
        ImGui::ColorEdit4("Diffuse#Light", &mDirectionalLight.diffuse.x);
        ImGui::ColorEdit4("Specular#Light", &mDirectionalLight.specular.x);
    }

    ImGui::Separator();

    Math::Vector3 pos = mFootball.transform.position;
    if (ImGui::DragFloat3("BallPosition", &pos.x))
    {
        mFootball.transform.position = pos;
        mBallRigidBody.SetPosition(mFootball.transform.position);
    }

    mStandardEffect.DebugUI();
    PhysicsWorld::Get()->DebugUI();
    ImGui::End();
    SimpleDraw::Render(mCamera);
}

void GameState::UpdateCamera(float deltaTime)
{
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
