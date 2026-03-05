#include "Precompiled.h"
#include "ParticleSystem.h"


#include <Graphics/Inc/ParticleSystemEffect.h>
using namespace Engine;
using namespace Engine::Physics;
using namespace Engine::Graphics;

void ParticleSystem::Initialize(const ParticleSystemInfo& info)
{
    mInfo = info;
    mNextAvailableParticleIndex = 0;
    mNextSpawnTime = info.delay;
    mLifeTime = info.lifeTime;

    InitializeParticles(info.maxParticles);
}

void ParticleSystem::Terminate()
{
    for (auto& particle : mParticles)
    {
        particle->Terminate();
        particle.reset();
    }
    mParticles.clear();

    TextureManager::Get()->ReleaseTexture(mInfo.textureId);
}

void ParticleSystem::Update(float deltaTime)
{
    if (IsActive())
    {
        mLifeTime -= deltaTime;
        mNextSpawnTime -= deltaTime;
        if (mNextSpawnTime <= 0.0f && mLifeTime > 0.0f)
        {
            SpawnParticles();
        }
        for (auto& particle : mParticles)
        {
            particle->Update(deltaTime);
        }
    }
}

bool ParticleSystem::IsActive()
{
    if (mLifeTime > 0.0f)
    {
        return true;
    }
    for (auto& particle : mParticles)
    {
        if (particle->IsActive())
        {
            return true;
        }
    }
    return false;
}

void ParticleSystem::DebugUI()
{
    ImGui::PushID(this);
    if (ImGui::CollapsingHeader("ParticleSystem"))
    {
        ImGui::DragFloat3("Position", &mInfo.spawnPosition.x);
        if (ImGui::DragFloat3("Direction", &mInfo.spawnDirection.x))
        {
            mInfo.spawnDirection = Math::Normalize(mInfo.spawnDirection);
        }
        ImGui::DragIntRange2("PerEmit", &mInfo.particlesPerEmit.min, &mInfo.particlesPerEmit.max);
        ImGui::DragFloatRange2(
            "TimeBetweenEmit", &mInfo.timeBetweenEmit.min, &mInfo.timeBetweenEmit.max);
        ImGui::DragFloatRange2("SpawnAngle", &mInfo.spawnAngle.min, &mInfo.spawnAngle.max);
        ImGui::DragFloatRange2("SpawnSpeed", &mInfo.spawnSpeed.min, &mInfo.spawnSpeed.max);
        ImGui::DragFloatRange2(
            "ParticleLifeTime", &mInfo.particleLifeTime.min, &mInfo.particleLifeTime.max);
        ImGui::DragFloat3("StartScaleMin", &mInfo.startScale.min.x);
        ImGui::DragFloat3("StartScaleMax", &mInfo.startScale.max.x);
        ImGui::DragFloat3("EndScaleMin", &mInfo.endScale.min.x);
        ImGui::DragFloat3("EndScaleMax", &mInfo.endScale.max.x);
        ImGui::ColorEdit4("StartColourMin", &mInfo.startColour.min.x);
        ImGui::ColorEdit4("StartColourMax", &mInfo.startColour.max.x);
        ImGui::ColorEdit4("EndColourMin", &mInfo.endColour.min.x);
        ImGui::ColorEdit4("EndColourMax", &mInfo.endColour.max.x);
    }
    ImGui::PopID();
}

void ParticleSystem::SetPositon(const Math::Vector3& position)
{
    mInfo.spawnPosition = position;
}

void ParticleSystem::SpawnParticles()
{
    int numParticles = mInfo.particlesPerEmit.GetRandomInc();
    for (int i = 0; i < numParticles; ++i)
    {
        SpawnSingleParticle();
    }
    mNextSpawnTime = mInfo.timeBetweenEmit.GetRandom();
}

void ParticleSystem::Render(Graphics::ParticleSystemEffect& effect)
{
    if (!IsActive())
    {
        return;
    }

    effect.SetTextureId(mInfo.textureId);
    for (auto& particle : mParticles)
    {
        if (particle->IsActive())
        {
            effect.Render(particle->GetTransform(), particle->GetColour());
        }
    }
}

void ParticleSystem::InitializeParticles(uint32_t maxParticles)
{
    mParticles.resize(maxParticles);
    for (uint32_t i = 0; i < maxParticles; ++i)
    {
        mParticles[i] = std::make_unique<Particle>();
        mParticles[i]->Initialize();
    }
}

void ParticleSystem::SpawnSingleParticle()
{
    Particle* particle = mParticles[mNextAvailableParticleIndex].get();
    mNextAvailableParticleIndex = (mNextAvailableParticleIndex + 1) % mInfo.maxParticles;

    const bool isUp = (Math::Abs(Math::Dot(mInfo.spawnDirection, Math::Vector3::YAxis))) > 0.9999f;
    Math::Vector3 r =
        (isUp) ? Math::Vector3::XAxis
               : Math::Normalize(Math::Cross(Math::Vector3::YAxis, mInfo.spawnDirection));

    Math::Vector3 u = Math::Normalize(Math::Cross(mInfo.spawnDirection, r));

    float rotAngle = mInfo.spawnAngle.GetRandom() * Math::Constants::DegToRad;
    Math::Matrix4 matRotRight = Math::Matrix4::RotationAxis(r, rotAngle);

    rotAngle = mInfo.spawnAngle.GetRandom() * Math::Constants::DegToRad;
    Math::Matrix4 matRotUp = Math::Matrix4::RotationAxis(u, rotAngle);

    Math::Matrix4 matRotation = matRotRight * matRotUp;

    Math::Vector3 spawnDirection = Math::TransformNormal(mInfo.spawnDirection, matRotation);

    const float speed = mInfo.spawnSpeed.GetRandom();
    ParticleInfo info;
    info.lifetime = mInfo.particleLifeTime.GetRandom();
    info.startColour = mInfo.startColour.GetRandom();
    info.endColour = mInfo.endColour.GetRandom();
    info.startScale = mInfo.startScale.GetRandom();
    info.endScale = mInfo.endScale.GetRandom();
    info.position = mInfo.spawnPosition;
    info.velocity = spawnDirection * speed;
    particle->Activate(info);
}
