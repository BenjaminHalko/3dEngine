#pragma once

#include "Particle.h"

namespace Engine::Graphics
{
class ParticleSystemEffect;
}

namespace Engine::Physics
{
struct ParticleSystemInfo
{
    Graphics::TextureId textureId = 0;
    int maxParticles = 1000;
    float delay = 0.0f;
    float lifeTime = 0.0f;
    Math::RangeInt particlesPerEmit = {0, 0};
    Math::Range<float> timeBetweenEmit = {0.0f, 0.0f};
    Math::Range<float> spawnAngle = {0.0f, 0.0f};
    Math::Range<float> spawnSpeed = {0.0f, 0.0f};
    Math::Range<float> particleLifeTime = {0.0f, 0.0f};
    Math::Vector3 spawnPosition = Math::Vector3::Zero;
    Math::Vector3 spawnDirection = Math::Vector3::YAxis;
    Math::Range<Math::Vector3> startScale = {Math::Vector3::One, Math::Vector3::One};
    Math::Range<Math::Vector3> endScale = {Math::Vector3::One, Math::Vector3::One};
    Math::Range<Graphics::Color> startColour = {Graphics::Colors::White, Graphics::Colors::White};
    Math::Range<Graphics::Color> endColour = {Graphics::Colors::White, Graphics::Colors::White};
};

class ParticleSystem
{
  public:
    void Initialize(const ParticleSystemInfo& info);
    void Terminate();
    void Update(float deltaTime);

    bool IsActive();
    void DebugUI();

    void SetPositon(const Math::Vector3& position);

    void SpawnParticles();
    void Render(Graphics::ParticleSystemEffect& effect);

  private:
    void InitializeParticles(uint32_t maxParticles);
    void SpawnSingleParticle();

    using Particles = std::vector<std::unique_ptr<Particle>>;
    Particles mParticles;

    ParticleSystemInfo mInfo;
    int mNextAvailableParticleIndex = 0;
    float mNextSpawnTime = 0.0f;
    float mLifeTime = 0.0f;
};
} // namespace Engine::Physics
