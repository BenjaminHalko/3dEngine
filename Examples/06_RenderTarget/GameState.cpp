#include "GameState.h"

using namespace Engine;
using namespace Engine::Graphics;
using namespace Engine::Input;

void GameState::Initialize()
{
    mCamera.SetPosition({0.0f, 1.0f, -3.0f});
    mCamera.SetLookAt({0.0f, 0.0f, 0.0f});

    mRenderTargetCamera.SetPosition({0.0f, 1.0f, -3.0f});
    mRenderTargetCamera.SetLookAt({0.0f, 0.0f, 0.0f});
    mRenderTargetCamera.SetAspectRatio(1.0f);

    // Initialize GPU Communication
    std::filesystem::path shaderFile = L"Assets/Shaders/DoTexture.hlsl";
    mVertexShader.Initialize<VertexPX>(shaderFile);
    mPixelShader.Initialize(shaderFile);
    mSampler.Initialize(Sampler::Filter::Linear, Sampler::AddressMode::Wrap);
    mTransformBuffer.Initialize(sizeof(Math::Matrix4));

    // Initialize Render Object
    MeshPX sphere = MeshBuilder::CreateSpherePX(60, 60, 1.0f);
    mEarth.meshBuffer.Initialize(sphere);
    mSun.meshBuffer.Initialize(sphere);
    MeshPX spaceSphere = MeshBuilder::CreateSkySpherePX(30, 30, 250.0f);
    mSpace.meshBuffer.Initialize(spaceSphere);

    mSpace.textureId = TextureManager::Get()->LoadTexture(L"space.jpg");
    mEarth.textureId = TextureManager::Get()->LoadTexture(L"earth.jpg");
    mSun.textureId = TextureManager::Get()->LoadTexture(L"sun.jpg");

    // Moving other objects to the right positions
    mSun.worldMat = Math::Matrix4::Translation(0.0f, 0.0f, 0.0f);
    mEarth.worldMat = Math::Matrix4::Translation(3.0f, 0.0f, 0.0f);
    mSpace.worldMat = Math::Matrix4::Translation(0.0f, 0.0f, 0.0f);

    constexpr uint32_t size = 512;
    mRenderTarget.Initialize(size, size, RenderTarget::Format::RGBA_U32);
}

void GameState::Terminate()
{
    TextureManager::Get()->ReleaseTexture(mSpace.textureId);
    TextureManager::Get()->ReleaseTexture(mEarth.textureId);
    TextureManager::Get()->ReleaseTexture(mSun.textureId);
    mSpace.meshBuffer.Terminate();
    mEarth.meshBuffer.Terminate();
    mSun.meshBuffer.Terminate();

    mTransformBuffer.Terminate();
    mSampler.Terminate();
    mPixelShader.Terminate();
    mVertexShader.Terminate();
    mRenderTarget.Terminate();
}

void GameState::Update(float deltaTime)
{
    UpdateCamera(deltaTime);
}

enum class PlanetRenderTargets
{
    Space,
    Earth,
    Sun
};
const char* gPlanetNames[] = {"Space", "Earth", "Sun"};
PlanetRenderTargets gCurrentPlanet = PlanetRenderTargets::Space;

void GameState::Render()
{
    SimpleDraw::AddGroundPlane(20.0f, Colors::Wheat);
    SimpleDraw::Render(mCamera);

    // Render to Render Target ImGui Image
    mRenderTarget.BeginRender();
    if (gCurrentPlanet == PlanetRenderTargets::Space)
    {
        RenderObject(mSpace, mRenderTargetCamera);
        mRenderTargetCamera.SetPosition({0.0f, 1.0f, -3.0f});
        mRenderTargetCamera.SetLookAt({0.0f, 0.0f, 0.0f});
    }
    else if (gCurrentPlanet == PlanetRenderTargets::Earth)
    {
        RenderObject(mEarth, mRenderTargetCamera);
        mRenderTargetCamera.SetPosition({0.0f, 1.0f, -3.0f});
        mRenderTargetCamera.SetLookAt({3.0f, 0.0f, 0.0f});
    }
    else if (gCurrentPlanet == PlanetRenderTargets::Sun)
    {
        RenderObject(mSun, mRenderTargetCamera);
        mRenderTargetCamera.SetPosition({0.0f, 1.0f, -3.0f});
        mRenderTargetCamera.SetLookAt({0.0f, 0.0f, 0.0f});
    }
    else
    {
        RenderObject(mSpace, mRenderTargetCamera);
    }
    mRenderTarget.EndRender();

    // Render to Scene
    RenderObject(mSpace, mCamera);
    RenderObject(mEarth, mCamera);
    RenderObject(mSun, mCamera);
}

void GameState::RenderObject(const Object& object, const Engine::Graphics::Camera& camera)
{
    const Math::Matrix4 matView = mCamera.GetViewMatrix();
    const Math::Matrix4 matProj = mCamera.GetProjectionMatrix();
    const Math::Matrix4 matFinal = object.worldMat * matView * matProj;
    const Math::Matrix4 wvp = Math::Transpose(matFinal);
    mTransformBuffer.Update(&wvp);

    mVertexShader.Bind();
    mPixelShader.Bind();
    mSampler.BindPS(0);
    mTransformBuffer.BindVS(0);

    TextureManager::Get()->BindPS(object.textureId, 0);
    object.meshBuffer.Render();
}

bool gCheckValue = false;
float gFloatVal = 0.0f;
Math::Vector3 gV0 = Math::Vector3::Zero;
Math::Vector3 gV1 = Math::Vector3::One;
Math::Vector3 gV2 = Math::Vector3::XAxis;
Color gColor = Colors::White;
// Sphere & Circle Variables:
int sphereSlices = 60;
int sphereRings = 60;
float sphereRadius = 1.0f;
Color sphereColor = Colors::White;
Math::Vector3 sphereOrigin = Math::Vector3::Zero;

enum class Shape
{
    None,
    AABB,
    AABBFilled,
    Sphere,
    GroundPlane,
    GroundCircle,
    Transform
};
const char* gShapeNames[] =
    {"None", "AABB", "AABBFilled", "Sphere", "GroundPlane", "GroundCircle", "Transform"};
Shape gCurrentShape = Shape::None;

void GameState::DebugUI()
{
    ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Im Skylar White, YO!");
    ImGui::Checkbox("My husband is Walter White, YO?", &gCheckValue);
    ImGui::DragFloat("How Bad are you Breaking?", &gFloatVal);
    ImGui::DragFloat3("[V0] So you do have a plan?", &gV0.x, 0.1f);
    ImGui::DragFloat3("[V1] Yeah, Mr.White!", &gV1.x, 0.1f);
    ImGui::DragFloat3("[V2] YEAH SCIENCE!", &gV2.x, 0.1f);
    ImGui::ColorEdit4("Walter...?", &gColor.r);

    int currentShape = (int) gCurrentShape;
    int currentPlanet = (int) gCurrentPlanet;

    if (ImGui::Combo("Shape", &currentShape, gShapeNames, std::size(gShapeNames)))
    {
        gCurrentShape = (Shape) currentShape;
    }

    switch (gCurrentShape)
    {
    case Shape::None:
        break;

    case Shape::AABB:
    {
        ImGui::Text("Non-Filled AABB Menu:");
        ImGui::DragFloat3("V0", &gV0.x, 0.1);
        ImGui::DragFloat3("V1", &gV1.x, 0.1);
        ImGui::ColorEdit4("Color", &gColor.r);

        SimpleDraw::AddAABB(gV0, gV1, gColor);
        break;
    }

    case Shape::AABBFilled:
    {
        ImGui::Text("Filled AABB Menu:");
        ImGui::DragFloat3("V0", &gV0.x, 0.1);
        ImGui::DragFloat3("V1", &gV1.x, 0.1);
        ImGui::ColorEdit4("Color", &gColor.r);

        SimpleDraw::AddFilledAABB(gV0, gV1, gColor);
        break;
    }

    case Shape::Sphere:
    {
        ImGui::Text("Sphere Menu:");
        ImGui::DragInt("Slices", &sphereSlices, 1, 3, 100);
        ImGui::DragInt("Rings", &sphereRings, 1, 3, 100);
        ImGui::DragFloat("Radius", &sphereRadius, 0.1f, 0.1f, 100.0f);
        ImGui::ColorEdit4("Color", &sphereColor.r);
        ImGui::DragFloat3("Origin", &sphereOrigin.x, 0.1f);

        SimpleDraw::AddSphere(sphereSlices, sphereRings, sphereRadius, sphereColor, sphereOrigin);
        break;
    }
    case Shape::GroundPlane:
    {
        ImGui::Text("Ground Plane Menu:");
        ImGui::DragFloat("Size", &gFloatVal, 1, 1, 100);
        ImGui::ColorEdit4("Color", &gColor.r);

        SimpleDraw::AddGroundPlane(gFloatVal, gColor);
        break;
    }

    case Shape::GroundCircle:
    {
        ImGui::Text("Ground Circle Menu:");
        ImGui::DragInt("Slices", &sphereSlices, 1, 3, 100);
        ImGui::DragFloat("Radius", &sphereRadius, 0.1f, 0.1f, 100.0f);
        ImGui::ColorEdit4("Color", &sphereColor.r);
        ImGui::DragFloat3("Origin", &sphereOrigin.x, 0.1f);

        SimpleDraw::AddGroundCircle(sphereSlices, sphereRadius, sphereColor, sphereOrigin);
        break;
    }

    case Shape::Transform:
    {
        SimpleDraw::AddTransform(Math::Matrix4::Identity);
        break;
    }
    }

    // To Render specific planet to the render target GUI Window
    if (ImGui::Combo("Planet Render Target", &currentPlanet, gPlanetNames, std::size(gPlanetNames)))
    {
        gCurrentPlanet = (PlanetRenderTargets) currentPlanet;
    }

    ImGui::Separator();
    ImGui::Text("RenderTarget");
    ImGui::Image(
        mRenderTarget.GetRawData(), {128, 128}, {0, 0}, {1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1});

    ImGui::End();
}

void GameState::UpdateCamera(float deltaTime)
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
