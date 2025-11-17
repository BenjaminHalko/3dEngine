#pragma once

#include "MeshBuffer.h"
#include "Transform.h"
#include "Material.h"
#include "TextureManager.h"
#include "ModelManager.h"

namespace Engine::Graphics
{
    class RenderObject
    {
    public:
        void Terminate();

        Transform transform;   // Location/ Orientation
        MeshBuffer meshBuffer; // Shape

        Material material;    // Light data

        TextureId diffuseMapId;  // Diffuse texture for an object
        TextureId specMapId;   // Specular map for an object
        TextureId normalMapId;   // Normal texture for an object
        TextureId bumpMapId;   // Height texture for an object
    };

    class RenderGroup
    {
    public:
        void Initialize(const std::filesystem::path& modelFilePath);
        void Terminate();

        ModelId modelId; // Model Identifier
        Transform transform; // Root Transform (Other objects may have other transforms)
        std::vector<RenderObject> renderObjects; // All objects to render
    };
} // namespace Engine::Graphics
