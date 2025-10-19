#pragma once

#include "MeshBuffer.h"
#include "Transform.h"
#include "Material.h"
#include "TextureManager.h"

namespace Engine::Graphics
{
class RenderObject
{
  public:
    void Terminate()
    {
        meshBuffer.Terminate();
        TextureManager* tm = TextureManager::Get();
        tm->ReleaseTexture(diffuseMapId);
        tm->ReleaseTexture(specMapId);
        tm->ReleaseTexture(normalMapId);
        tm->ReleaseTexture(bumpMapId);
    }

    Transform transform;   // Location/ Orientation
    MeshBuffer meshBuffer; // Shape

    Material material; // Light data

    TextureId diffuseMapId = 0; // Diffuse texture for an object
    TextureId specMapId = 0;    // Specular map for an object
    TextureId normalMapId = 0;  // Normal texture for an object
    TextureId bumpMapId = 0;    // Height texture for an object
};
} // namespace Engine::Graphics
