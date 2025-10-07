#pragma once

#include "MeshBuffer.h"
#include "Texture.h"
#include "Transform.h"
#include "Material.h"

namespace Engine::Graphics
{
struct RenderObject
{
    MeshBuffer meshBuffer;
    Texture texture;
    Material material;
    Transform transform;

    void Terminate()
    {
        meshBuffer.Terminate();
        texture.Terminate();
    }
};
}