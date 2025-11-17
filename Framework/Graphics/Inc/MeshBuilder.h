#pragma once

#include "MeshTypes.h"

namespace Engine::Graphics
{
class MeshBuilder
{
  public:
    // Cube
    static MeshPC CreateCubePC(float size, const Color& color);
    static MeshPC CreateCubePC(float size);
    static MeshPX CreateCubePX(float size);

    // Pyramid
    static MeshPC CreatePyramidPC(float size);

    // Rectangle
    static MeshPC CreateRectanglePC(float width, float height, float depth);

    // Plane
    static MeshPC CreatePlanePC(int numRows, int numColums, float spacing, bool horizontal = true);
    static MeshPX CreatePlanePX(int numRows, int numColums, float spacing, bool horizontal = true);
    static Mesh CreatePlane(int numRows, int numColums, float spacing, bool horizontal = true);

    // Cylinder
    static MeshPC CreateCylinderPC(int slices, int rings);

    // Sphere
    static MeshPC CreateSpherePC(int slices, int rings, float radius);
    static MeshPX CreateSpherePX(int slices, int rings, float radius);
    static Mesh CreateSphere(int slices, int rings, float radius);

    // Sky Sphere/ 'Box'
    static MeshPX CreateSkySpherePX(int slices, int rings, float radius);
    static Mesh CreateSkySphere(int slices, int rings, float radius);

    // Load 3D Obj
    static MeshPX CreateOBJPX(const std::filesystem::path& filePath, float scale);

    // Screen Quad
    static MeshPX CreateScreenQuadPX();
};
} // namespace Engine::Graphics
