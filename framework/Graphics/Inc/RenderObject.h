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
        void Terminate();

        Transform transform;
        MeshBuffer meshBuffer;
        Material material;
        TextureId diffuseMapId;
        TextureId specMapId;
        TextureId normalMapId;
        TextureId bumpMapId;
    };
}