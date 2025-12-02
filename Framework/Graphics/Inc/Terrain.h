#pragma once

#include "Common.h"
#include "MeshTypes.h"

namespace Engine::Graphics
{
class Terrain final
{
  public:
    void Initialize(const std::filesystem::path& fileName, float heightScale);

    float GetHeight(const Math::Vector3& position) const;

    Mesh mesh;
    uint32_t rows = 0;
    uint32_t columns = 0;
};
} // namespace Engine::Graphics
