#include "GameState.h"

using namespace Engine;
using namespace Engine::Graphics;

void GameState::Initialize() {
    // Initialize cameras
    mMainCamera.SetPosition({0.0f, 50.0f, -100.0f});
    mMainCamera.SetLookAt({0.0f, 0.0f, 0.0f});
    mPlanetCamera.SetPosition({0.0f, 0.0f, -10.0f});
    mPlanetCamera.SetLookAt({0.0f, 0.0f, 0.0f});

    // Initialize GPU Communication
    std::filesystem::path shaderFile = L"Assets/Shaders/DoTexture.fx";
    mVertexShader.Initialize<VertexPX>(shaderFile);
    mPixelShader.Initialize(shaderFile);
    mSampler.Initialize(Sampler::Filter::Linear, Sampler::AddressMode::Wrap);
    mTransformBuffer.Initialize(sizeof(Math::Matrix4));

    // Initialize render target for planet view
    mPlanetRenderTarget.Initialize(512, 512, RenderTarget::Format::RGBA_U8);

    // Initialize UI state
    mSelectedPlanetIndex = 0;
    mShowOrbits = true;
    mGlobalSpeedMultiplier = 1.0f;
    mShowPlanetView = true;

    // Create sky sphere
    CreateSkySphere();

    // Create sun
    CreateSun();

    // Create planets
    CreatePlanets();
}

void GameState::Terminate() {
    mPlanetRenderTarget.Terminate();
    mSkySphere.Terminate();
    mSun.mesh.Terminate();
    mSun.texture.Terminate();
    for (auto &planet : mPlanets) {
        planet.mesh.Terminate();
        planet.texture.Terminate();
        if (planet.moon) {
            planet.moon->mesh.Terminate();
            planet.moon->texture.Terminate();
        }
    }

    mTransformBuffer.Terminate();
    mSampler.Terminate();
    mPixelShader.Terminate();
    mVertexShader.Terminate();
}

void GameState::CreateSkySphere() {
    // Create a large sphere for the sky
    mSkySphere.Initialize(MeshBuilder::CreateSkySpherePX(32, 32, 1000.0f));
    mSkyTexture.Initialize("../../Assets/Textures/space.jpg");
}

void GameState::CreateSun() {
    mSun.name = "Sun";
    mSun.mesh.Initialize(MeshBuilder::CreateSpherePX(32, 32, 10.0f));
    mSun.texture.Initialize("../../Assets/Textures/sun.jpg");
    mSun.orbitRadius = 0.0f;
    mSun.orbitSpeed = 0.0f;
    mSun.rotationSpeed = 0.1f;
    mSun.orbitAngle = 0.0f;
    mSun.rotationAngle = 0.0f;
    mSun.showOrbit = false;
}

void GameState::CreatePlanets() {
    // Mercury
    mPlanets.push_back(CreatePlanet("Mercury", 5.0f, 0.387f, 0.24f, 58.6f, "mercury.jpg"));

    // Venus
    mPlanets.push_back(CreatePlanet("Venus", 6.0f, 0.723f, 0.62f, -243.0f, "venus.jpg"));

    // Earth
    auto &earth = CreatePlanet("Earth", 7.0f, 1.0f, 1.0f, 1.0f, "earth.jpg");
    // Add moon
    earth.moon = std::make_unique<CelestialBody>();
    earth.moon->name = "Moon";
    earth.moon->mesh.Initialize(MeshBuilder::CreateSpherePX(32, 32, 1.5f));
    earth.moon->texture.Initialize("../../Assets/Textures/moon.jpg");
    earth.moon->orbitRadius = 3.0f;
    earth.moon->orbitSpeed = 27.3f;
    earth.moon->rotationSpeed = 27.3f;
    earth.moon->orbitAngle = 0.0f;
    earth.moon->rotationAngle = 0.0f;
    earth.moon->showOrbit = true;
    mPlanets.push_back(std::move(earth));

    // Mars
    mPlanets.push_back(CreatePlanet("Mars", 5.5f, 1.524f, 1.88f, 1.03f, "mars.jpg"));

    // Jupiter
    mPlanets.push_back(CreatePlanet("Jupiter", 15.0f, 5.203f, 11.86f, 0.41f, "jupiter.jpg"));

    // Saturn
    mPlanets.push_back(CreatePlanet("Saturn", 12.0f, 9.537f, 29.46f, 0.45f, "saturn.jpg"));

    // Uranus
    mPlanets.push_back(CreatePlanet("Uranus", 8.0f, 19.191f, 84.01f, -0.72f, "uranus.jpg"));

    // Neptune
    mPlanets.push_back(CreatePlanet("Neptune", 8.0f, 30.069f, 164.79f, 0.67f, "neptune.jpg"));

    // Pluto
    mPlanets.push_back(CreatePlanet("Pluto", 3.0f, 39.482f, 248.54f, -6.39f, "pluto.jpg"));
}

CelestialBody GameState::CreatePlanet(const std::string &name, float size, float orbitRadius,
                                      float orbitSpeed, float rotationSpeed,
                                      const std::string &textureFile) {
    CelestialBody planet;
    planet.name = name;
    planet.mesh.Initialize(MeshBuilder::CreateSpherePX(32, 32, size));
    planet.texture.Initialize("../../Assets/Textures/" + textureFile);
    planet.orbitRadius = orbitRadius * 10.0f; // Scale up for better visualization
    planet.orbitSpeed = orbitSpeed;
    planet.rotationSpeed = rotationSpeed;
    planet.orbitAngle = 0.0f;
    planet.rotationAngle = 0.0f;
    planet.showOrbit = true;
    planet.moon = nullptr; // Explicitly initialize moon pointer
    return planet;
}

void GameState::Update(float deltaTime) {
    //     // Update sun rotation
    //     mSun.rotationAngle += mSun.rotationSpeed * deltaTime * mGlobalSpeedMultiplier;
    //     mSun.transform.rotation =
    //         Math::Quaternion::CreateFromYawPitchRoll(mSun.rotationAngle, 0.0f, 0.0f);

    //     // Update planets
    //     for (auto &planet : mPlanets) {
    //         UpdateCelestialBody(planet, deltaTime);
    //     }
}

void GameState::UpdateCelestialBody(CelestialBody &body, float deltaTime) {
    // Update orbit
    body.orbitAngle += body.orbitSpeed * deltaTime * mGlobalSpeedMultiplier;
    float x = body.orbitRadius * cos(body.orbitAngle);
    float z = body.orbitRadius * sin(body.orbitAngle);
    body.transform.position = {x, 0.0f, z};

    // Update rotation
    body.rotationAngle += body.rotationSpeed * deltaTime * mGlobalSpeedMultiplier;
    body.transform.rotation =
        Math::Quaternion::CreateFromYawPitchRoll(body.rotationAngle, 0.0f, 0.0f);

    // Update moon if exists
    if (body.moon) {
        body.moon->orbitAngle += body.moon->orbitSpeed * deltaTime * mGlobalSpeedMultiplier;
        float moonX = body.moon->orbitRadius * cos(body.moon->orbitAngle);
        float moonZ = body.moon->orbitRadius * sin(body.moon->orbitAngle);
        body.moon->transform.position = body.transform.position + Math::Vector3{moonX, 0.0f, moonZ};
        body.moon->rotationAngle += body.moon->rotationSpeed * deltaTime * mGlobalSpeedMultiplier;
        body.moon->transform.rotation =
            Math::Quaternion::CreateFromYawPitchRoll(body.moon->rotationAngle, 0.0f, 0.0f);
    }
}

void GameState::RenderObject(const CelestialBody &body, const Camera &camera) {
    const Math::Matrix4 matView = camera.GetViewMatrix();
    const Math::Matrix4 matProj = camera.GetProjectionMatrix();
    const Math::Matrix4 matFinal = body.transform.Apply() * matView * matProj;
    const Math::Matrix4 wvp = Math::Transpose(matFinal);
    mTransformBuffer.Update(&wvp);

    mVertexShader.Bind();
    mPixelShader.Bind();
    mSampler.BindPS(0);
    mTransformBuffer.BindVS(0);

    body.texture.BindPS(0);
    body.mesh.Render();
}

void GameState::Render() {
    // Render sky sphere
    mSkyTexture.BindPS(0);
    auto skyTransform = Math::Matrix4::Identity;
    SimpleDraw::AddTransform(skyTransform);
    mSkySphere.Render();

    // Render sun
    RenderObject(mSun, mMainCamera);

    // Render planets
    for (const auto &planet : mPlanets) {
        RenderObject(planet, mMainCamera);

        // Render moon if exists
        if (planet.moon) {
            RenderObject(*planet.moon, mMainCamera);
        }

        // Draw orbit if enabled
        if (mShowOrbits && planet.showOrbit) {
            DrawOrbit(planet);
        }
    }

    // Render planet view if enabled
    if (mShowPlanetView && mSelectedPlanetIndex >= 0 && mSelectedPlanetIndex < mPlanets.size()) {
        RenderPlanetView();
    }
}

void GameState::DrawOrbit(const CelestialBody &body) {
    const int segments = 100;
    const float angleStep = Math::Constants::TwoPi / segments;

    for (int i = 0; i < segments; ++i) {
        float angle1 = i * angleStep;
        float angle2 = (i + 1) * angleStep;

        float x1 = body.orbitRadius * cos(angle1);
        float z1 = body.orbitRadius * sin(angle1);
        float x2 = body.orbitRadius * cos(angle2);
        float z2 = body.orbitRadius * sin(angle2);

        SimpleDraw::AddLine({x1, 0.0f, z1}, {x2, 0.0f, z2}, Colors::White);
    }
}

void GameState::RenderPlanetView() {
    if (mSelectedPlanetIndex < 0 || mSelectedPlanetIndex >= mPlanets.size())
        return;

    const auto &planet = mPlanets[mSelectedPlanetIndex];

    // Update planet camera position
    Math::Vector3 cameraPos = planet.transform.position + Math::Vector3{0.0f, 0.0f, -20.0f};
    mPlanetCamera.SetPosition(cameraPos);
    mPlanetCamera.SetLookAt(planet.transform.position);

    // Render to planet view render target
    mPlanetRenderTarget.BeginRender();
    {
        // Clear the render target
        GraphicsSystem::Get()->BeginRender();

        // Render planet
        RenderObject(planet, mPlanetCamera);

        // Render moon if exists
        if (planet.moon) {
            RenderObject(*planet.moon, mPlanetCamera);
        }
    }
    mPlanetRenderTarget.EndRender();
}

void GameState::DebugUI() {
    ImGui::Begin("Solar System Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    // Global controls
    ImGui::Checkbox("Show Orbits", &mShowOrbits);
    ImGui::SliderFloat("Global Speed", &mGlobalSpeedMultiplier, 0.0f, 5.0f);
    ImGui::Checkbox("Show Planet View", &mShowPlanetView);

    // Planet selection
    const char *planetNames[] = {"Mercury", "Venus",  "Earth",   "Mars", "Jupiter",
                                 "Saturn",  "Uranus", "Neptune", "Pluto"};
    ImGui::Combo("Select Planet", &mSelectedPlanetIndex, planetNames, IM_ARRAYSIZE(planetNames));

    // Planet view
    if (mShowPlanetView && mSelectedPlanetIndex >= 0 && mSelectedPlanetIndex < mPlanets.size()) {
        ImGui::BeginChild("PlanetView", ImVec2(512, 512), true);
        ImGui::Image(mPlanetRenderTarget.GetRawData(), ImVec2(512, 512));
        ImGui::EndChild();
    }

    // Individual planet controls
    if (mSelectedPlanetIndex >= 0 && mSelectedPlanetIndex < mPlanets.size()) {
        auto &planet = mPlanets[mSelectedPlanetIndex];
        ImGui::Separator();
        ImGui::Text("Planet: %s", planet.name.c_str());
        ImGui::SliderFloat("Orbit Speed", &planet.orbitSpeed, 0.0f, 2.0f);
        ImGui::SliderFloat("Rotation Speed", &planet.rotationSpeed, -2.0f, 2.0f);
        ImGui::Checkbox("Show Orbit", &planet.showOrbit);
    }

    ImGui::End();
}
