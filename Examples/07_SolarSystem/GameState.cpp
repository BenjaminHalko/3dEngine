#include "GameState.h"

using namespace Engine;
using namespace Engine::Graphics;
using namespace Engine::Input;

void GameState::Initialize()
{
    // Initialize GPU Communication
    std::filesystem::path shaderFile = L"Assets/Shaders/DoTexture.hlsl";
    mVertexShader.Initialize<VertexPX>(shaderFile);
    mPixelShader.Initialize(shaderFile);
    mSampler.Initialize(Sampler::Filter::Linear, Sampler::AddressMode::Wrap);
    mTransformBuffer.Initialize(sizeof(Math::Matrix4));

    // Initialize render target for planet view
    mPlanetRenderTarget.Initialize(512, 512, RenderTarget::Format::RGBA_U8);

    // Initialize UI state
    mSelectedPlanetIndex = 0;
    mShowOrbits = true;
    mGlobalSpeedMultiplier = 0.01f;
    mShowPlanetView = true;

    // Create sky sphere
    MeshPX spaceSphere = MeshBuilder::CreateSkySpherePX(30, 30, 350.0f);
    mSkySphere.mesh.Initialize(spaceSphere);
    mSkySphere.textureId = TextureManager::Get()->LoadTexture(L"space.jpg");
    mSkySphere.matWorld = Math::Matrix4::Identity;

    // Create sun
    constexpr float visualScale = 0.1f; // scale down real ratios for visualization
    float sunRadius = 109.18f * visualScale;
    MeshPX sunSphere = MeshBuilder::CreateSpherePX(32, 32, sunRadius);
    mSun.mesh.Initialize(sunSphere);
    mSun.textureId = TextureManager::Get()->LoadTexture(L"sun.jpg");
    mSun.matWorld = Math::Matrix4::Identity;
    float orbitOffset = sunRadius;

    // Create planets
    const std::vector<std::tuple<std::string, float, float, float, float, std::wstring>>
        planetData = {
            std::make_tuple("Mercury",
                            0.383f * visualScale,
                            0.387f,
                            0.24f * 2.0f,
                            58.6f,
                            L"planets/mercury.jpg"),
            std::make_tuple(
                "Venus", 0.949f * visualScale, 0.723f, 0.62f * 2.0f, -243.0f, L"planets/venus.jpg"),
            std::make_tuple(
                "Earth", 1.0f * visualScale, 1.0f, 1.0f * 2.0f, 1.0f, L"planets/earth/earth.jpg"),
            std::make_tuple(
                "Mars", 0.532f * visualScale, 1.524f, 1.88f * 2.0f, 1.03f, L"planets/mars.jpg"),
            std::make_tuple("Jupiter",
                            11.21f * visualScale,
                            5.203f,
                            11.86f * 2.0f,
                            0.41f,
                            L"planets/jupiter.jpg"),
            std::make_tuple(
                "Saturn", 9.45f * visualScale, 9.537f, 29.46f * 2.0f, 0.45f, L"planets/saturn.jpg"),
            std::make_tuple("Uranus",
                            4.01f * visualScale,
                            19.191f,
                            84.01f * 2.0f,
                            -0.72f,
                            L"planets/uranus.jpg"),
            std::make_tuple("Neptune",
                            3.88f * visualScale,
                            30.069f,
                            164.79f * 2.0f,
                            0.67f,
                            L"planets/neptune.jpg"),
            std::make_tuple("Pluto",
                            0.187f * visualScale,
                            39.482f,
                            248.54f * 2.0f,
                            -6.39f,
                            L"planets/pluto.jpg")};

    constexpr float orbitScale = 10.0f; // scale AU distances for visualization

    for (const auto& [name, size, orbitRadius, orbitSpeed, rotationSpeed, textureFile] : planetData)
    {
        PlanetData planet;
        MeshPX sphere = MeshBuilder::CreateSpherePX(32, 32, size);
        planet.object.mesh.Initialize(sphere);
        planet.object.textureId = TextureManager::Get()->LoadTexture(textureFile);
        planet.orbitRadius = orbitOffset + orbitRadius * orbitScale;
        planet.orbitSpeed = orbitSpeed;
        planet.rotationSpeed = rotationSpeed;
        planet.object.matWorld = Math::Matrix4::RotationY(planet.orbitAngle) *
                                 Math::Matrix4::Translation(planet.orbitRadius, 0.0f, 0.0f) *
                                 Math::Matrix4::RotationY(planet.rotationAngle);
        planet.radius = size;
        mPlanets.push_back(std::move(planet));

        // Add moon for Earth
        if (name == "Earth")
        {
            auto moon = std::make_unique<PlanetObject>();
            MeshPX moonSphere = MeshBuilder::CreateSpherePX(32, 32, 0.2724f * visualScale);
            moon->mesh.Initialize(moonSphere);
            moon->textureId = TextureManager::Get()->LoadTexture(L"planets/pluto.jpg");
            moon->matWorld = Math::Matrix4::Translation(2.5f, 0.0f, 0.0f);
            mMoons.push_back(std::move(moon));
        }
    }

    // Initialize cameras
    // Start camera near Earth's orbit, looking at Earth
    constexpr int earthIndex = 2; // Earth is the third planet
    float earthOrbit = orbitOffset + 1.0f * orbitScale;
    mMainCamera.SetPosition({earthOrbit + 0.5f, 0.2f, 0.0});
    mMainCamera.SetLookAt({0.0f, 0.0f, 0.0f});
    mPlanetCamera.SetPosition({0.0f, 0.0f, -10.0f});
    mPlanetCamera.SetLookAt({0.0f, 0.0f, 0.0f});
    mPlanetCamera.SetAspectRatio(1.0f);
}

void GameState::Terminate()
{
    mPlanetRenderTarget.Terminate();

    // Release textures
    TextureManager::Get()->ReleaseTexture(mSkySphere.textureId);
    TextureManager::Get()->ReleaseTexture(mSun.textureId);

    // Terminate mesh buffers
    mSkySphere.mesh.Terminate();
    mSun.mesh.Terminate();
    for (auto& planet : mPlanets)
    {
        TextureManager::Get()->ReleaseTexture(planet.object.textureId);
        planet.object.mesh.Terminate();
    }
    for (auto& moon : mMoons)
    {
        TextureManager::Get()->ReleaseTexture(moon->textureId);
        moon->mesh.Terminate();
    }

    // Terminate GPU components
    mTransformBuffer.Terminate();
    mVertexShader.Terminate();
    mPixelShader.Terminate();
    mSampler.Terminate();
}

void GameState::Update(float deltaTime)
{
    UpdateCamera(deltaTime);

    // Update sun rotation
    mSun.matWorld = Math::Matrix4::RotationY(deltaTime * 0.1f * mGlobalSpeedMultiplier);

    // Update planets
    for (size_t i = 0; i < mPlanets.size(); ++i)
    {
        UpdateCelestialBody(mPlanets[i], deltaTime);
        if (i == 2 && !mMoons.empty())
        { // Earth's moon
            // Update moon orbit and rotation around Earth
            float moonOrbitSpeed = 4.0f;
            float moonRotationSpeed = 2.0f;
            static float moonOrbitAngle = 0.0f;
            static float moonRotationAngle = 0.0f;
            moonOrbitAngle += deltaTime * moonOrbitSpeed * mGlobalSpeedMultiplier;
            moonRotationAngle += deltaTime * moonRotationSpeed * mGlobalSpeedMultiplier;
            Math::Matrix4 moonOrbit = Math::Matrix4::RotationY(moonOrbitAngle) *
                                      Math::Matrix4::Translation(1.5f, 0.0f, 0.0f);
            Math::Matrix4 moonRotation = Math::Matrix4::RotationY(moonRotationAngle);
            mMoons[0]->matWorld = moonRotation * moonOrbit * mPlanets[2].object.matWorld;
        }
    }
}

void GameState::UpdateCelestialBody(PlanetData& body, float deltaTime)
{
    // Update orbit and rotation angles
    body.orbitAngle += deltaTime * body.orbitSpeed * mGlobalSpeedMultiplier;
    body.rotationAngle += deltaTime * body.rotationSpeed * mGlobalSpeedMultiplier;

    body.object.matWorld = Math::Matrix4::RotationY(body.orbitAngle) *
                           Math::Matrix4::Translation(body.orbitRadius, 0.0f, 0.0f) *
                           Math::Matrix4::RotationY(body.rotationAngle);
}

void GameState::DrawOrbit(const PlanetData& body)
{
    const int segments = 100;
    const float angleStep = Math::Constants::TwoPi / segments;
    float radius = body.orbitRadius;
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = i * angleStep;
        float angle2 = (i + 1) * angleStep;

        float x1 = radius * cos(angle1);
        float z1 = radius * sin(angle1);
        float x2 = radius * cos(angle2);
        float z2 = radius * sin(angle2);

        SimpleDraw::AddLine({x1, 0.0f, z1}, {x2, 0.0f, z2}, Colors::White);
    }
}

void GameState::Render()
{
    SimpleDraw::AddGroundPlane(20.0f, Colors::Wheat);
    SimpleDraw::Render(mMainCamera);

    // Render to Render Target
    mPlanetRenderTarget.BeginRender();
    if (mSelectedPlanetIndex >= 0 && mSelectedPlanetIndex < mPlanets.size())
    {
        // Render the selected planet at the origin for the preview
        RenderMeshAtOrigin(mPlanets[mSelectedPlanetIndex].object, mPlanetCamera);
        // If Earth is selected, render the moon orbiting it in the preview
        if (mSelectedPlanetIndex == 2 && !mMoons.empty())
        {
            float planetRadius = mPlanets[mSelectedPlanetIndex].radius;
            float moonOrbit = planetRadius * 2.5f;
            float moonAngle = ImGui::GetTime();
            Math::Matrix4 matWorld = Math::Matrix4::Translation(0.0f, 0.0f, 0.0f) *
                                     Math::Matrix4::RotationY(moonAngle) *
                                     Math::Matrix4::Translation(moonOrbit, 0.0f, 0.0f);
            const Math::Matrix4 matView = mPlanetCamera.GetViewMatrix();
            const Math::Matrix4 matProj = mPlanetCamera.GetProjectionMatrix();
            const Math::Matrix4 matFinal = matWorld * matView * matProj;
            const Math::Matrix4 wvp = Math::Transpose(matFinal);
            mTransformBuffer.Update(&wvp);
            mVertexShader.Bind();
            mPixelShader.Bind();
            mSampler.BindPS(0);
            mTransformBuffer.BindVS(0);
            TextureManager::Get()->BindPS(mMoons[0]->textureId, 0);
            mMoons[0]->mesh.Render();
        }
    }
    mPlanetRenderTarget.EndRender();

    // Render to Scene
    RenderMesh(mSkySphere, mMainCamera);
    RenderMesh(mSun, mMainCamera);
    for (size_t i = 0; i < mPlanets.size(); ++i)
    {
        RenderMesh(mPlanets[i].object, mMainCamera);
        if (mShowOrbits)
        {
            DrawOrbit(mPlanets[i]);
        }
        if (i == 2 && !mMoons.empty())
        { // Earth's moon
            RenderMesh(*mMoons[0], mMainCamera);
        }
    }
}

void GameState::RenderMesh(const PlanetObject& object, const Camera& camera)
{
    const Math::Matrix4 matView = camera.GetViewMatrix();
    const Math::Matrix4 matProj = camera.GetProjectionMatrix();
    const Math::Matrix4 matFinal = object.matWorld * matView * matProj;
    const Math::Matrix4 wvp = Math::Transpose(matFinal);
    mTransformBuffer.Update(&wvp);

    mVertexShader.Bind();
    mPixelShader.Bind();
    mSampler.BindPS(0);
    mTransformBuffer.BindVS(0);

    TextureManager::Get()->BindPS(object.textureId, 0);
    object.mesh.Render();
}

// Helper to render a planet at the origin for ImGui preview
void GameState::RenderMeshAtOrigin(const PlanetObject& object, const Camera& camera)
{
    // Make the planet spin in the preview
    float spin = ImGui::GetTime();
    const Math::Matrix4 matWorld = Math::Matrix4::RotationY(spin);
    const Math::Matrix4 matView = camera.GetViewMatrix();
    const Math::Matrix4 matProj = camera.GetProjectionMatrix();
    const Math::Matrix4 matFinal = matWorld * matView * matProj;
    const Math::Matrix4 wvp = Math::Transpose(matFinal);
    mTransformBuffer.Update(&wvp);

    mVertexShader.Bind();
    mPixelShader.Bind();
    mSampler.BindPS(0);
    mTransformBuffer.BindVS(0);

    TextureManager::Get()->BindPS(object.textureId, 0);
    object.mesh.Render();
}

void GameState::DebugUI()
{
    ImGui::Begin("Solar System Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    // Global controls
    ImGui::Checkbox("Show Orbits", &mShowOrbits);
    ImGui::SliderFloat("Global Speed", &mGlobalSpeedMultiplier, 0.0f, 1.0f);
    ImGui::Checkbox("Show Planet View", &mShowPlanetView);

    // Planet selection
    const char* planetNames[] = {
        "Mercury", "Venus", "Earth", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune", "Pluto"};
    static int planetCount = 9;
    ImGui::Combo("Select Planet", &mSelectedPlanetIndex, planetNames, planetCount);

    // Planet view
    if (mShowPlanetView && mSelectedPlanetIndex >= 0 && mSelectedPlanetIndex < planetCount)
    {
        float planetRadius = mPlanets[mSelectedPlanetIndex].radius;
        float camDist = planetRadius * 2.5f + 0.5f;
        mPlanetCamera.SetPosition({0.0f, 0.0f, -camDist});
        mPlanetCamera.SetLookAt({0.0f, 0.0f, 0.0f});
        ImGui::BeginChild("PlanetView", ImVec2(512, 512), true);
        ImGui::Image(mPlanetRenderTarget.GetRawData(), ImVec2(512, 512));
        ImGui::EndChild();
    }

    ImGui::End();
}

void GameState::UpdateCamera(float deltaTime)
{
    // Camera Controls:
    InputSystem* input = InputSystem::Get();
    const float moveSpeed = input->IsKeyDown(KeyCode::LSHIFT) ? 30.0f : 10.0f;
    const float turnSpeed = 0.5f;

    if (input->IsKeyDown(KeyCode::W))
    {
        mMainCamera.Walk(moveSpeed * deltaTime);
    }
    else if (input->IsKeyDown(KeyCode::S))
    {
        mMainCamera.Walk(-moveSpeed * deltaTime);
    }
    else if (input->IsKeyDown(KeyCode::D))
    {
        mMainCamera.Strafe(moveSpeed * deltaTime);
    }
    else if (input->IsKeyDown(KeyCode::A))
    {
        mMainCamera.Strafe(-moveSpeed * deltaTime);
    }
    else if (input->IsKeyDown(KeyCode::E))
    {
        mMainCamera.Rise(moveSpeed * deltaTime);
    }
    else if (input->IsKeyDown(KeyCode::Q))
    {
        mMainCamera.Rise(-moveSpeed * deltaTime);
    }

    if (input->IsMouseDown(MouseButton::RBUTTON))
    {
        mMainCamera.Yaw(input->GetMouseMoveX() * turnSpeed * deltaTime);
        mMainCamera.Pitch(input->GetMouseMoveY() * turnSpeed * deltaTime);
    }
}
