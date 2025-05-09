#pragma once

#include "Common.h"
#include "Color.h"

namespace Engine::Graphics
{
    // Vertex Element Flag
    // 0000 0000 0000
    constexpr uint32_t VE_Position   = 0x1 << 0;
    constexpr uint32_t VE_Normal     = 0x1 << 1;
    constexpr uint32_t VE_Tangent    = 0x1 << 2;
    constexpr uint32_t VE_Color      = 0x1 << 3;
    constexpr uint32_t VE_TexCoord   = 0x1 << 4;

#define VERTEX_FORMAT(fmt)\
static constexpr uint32_t Format = fmt

    struct VertexP
    {
        VERTEX_FORMAT(VE_Position);
        Math::Vector3 position;
    };

    struct VertexPC
    {
        VERTEX_FORMAT(VE_Position | VE_Color);
        Math::Vector3 position;
        Color color;
    };

    struct VertexPX
    {
        VERTEX_FORMAT(VE_Position | VE_TexCoord);
        Math::Vector3 position;
        Math::Vector2 uvCoord;
    };

    struct Vertex
    {
        VERTEX_FORMAT(VE_Position | VE_Normal | VE_Tangent | VE_TexCoord);
        Math::Vector3 position;
        Math::Vector3 normal;
        Math::Vector3 tangent;
        Math::Vector2 uvCoord;
    };
}
