#include "Precompiled.h"
#include "ModelIO.h"
#include "Model.h"

using namespace Engine;
using namespace Engine::Graphics;

void ModelIO::SaveModel(std::filesystem::path filePath, const Model& model)
{
    if (model.meshData.empty())
    {
        // LOG("Model has no mesh data to save.");
        return;
    }

    filePath.replace_extension("model");

    FILE* file = nullptr;
    fopen_s(&file, filePath.u8string().c_str(), "w");
    if (file == nullptr)
    {
        return;
    }

    const uint32_t meshCount = static_cast<uint32_t>(model.meshData.size());
    // Write file
    fprintf_s(file, "MeshCount: %d\n", meshCount);
    for (uint32_t m = 0; m < meshCount; ++m)
    {
        const Model::MeshData& meshData = model.meshData[m];
        fprintf_s(file, "MaterialIndex: %d\n", meshData.materialIndex);

        const Mesh& mesh = meshData.mesh;
        const uint32_t vertexCount = static_cast<uint32_t>(mesh.vertices.size());
        fprintf_s(file, "VertexCount: %d\n", vertexCount);
        for (const Vertex& v : mesh.vertices)
        {
            fprintf_s(file, "%f %f %f %f %f %f %f %f %f %f %f\n",
                v.position.x, v.position.y, v.position.z,
                v.normal.x, v.normal.y, v.normal.z,
                v.tangent.x, v.tangent.y, v.tangent.z,
                v.uvCoord.x, v.uvCoord.y);
        }

        const uint32_t indexCount = static_cast<uint32_t>(mesh.indices.size());
        fprintf_s(file, "IndexCount: %d\n", indexCount);
        for (uint32_t i = 2; i < indexCount; i += 3)
        {
            fprintf_s(file, "%d %d %d\n",
                mesh.indices[i - 2],
                mesh.indices[i - 1],
                mesh.indices[i]);
        }
    }
    fclose(file);
}

void ModelIO::LoadModel(std::filesystem::path filePath, Model& model)
{
    filePath.replace_extension("model");

    FILE* file = nullptr;
    fopen_s(&file, filePath.u8string().c_str(), "r");
    if (file == nullptr)
    {
        return;
    }

    uint32_t meshCount = 0;
    // Read file
    fscanf_s(file, "MeshCount: %d\n", &meshCount);
    model.meshData.resize(meshCount);
    for (uint32_t m = 0; m < meshCount; ++m)
    {
        Model::MeshData& meshData = model.meshData[m];
        fscanf_s(file, "MaterialIndex: %d\n", &meshData.materialIndex);

        Mesh& mesh = meshData.mesh;
        uint32_t vertexCount = 0;
        fscanf_s(file, "VertexCount: %d\n", &vertexCount);
        mesh.vertices.resize(vertexCount);
        for (Vertex& v : mesh.vertices)
        {
            fscanf_s(file, "%f %f %f %f %f %f %f %f %f %f %f\n",
                &v.position.x, &v.position.y, &v.position.z,
                &v.normal.x, &v.normal.y, &v.normal.z,
                &v.tangent.x, &v.tangent.y, &v.tangent.z,
                &v.uvCoord.x, &v.uvCoord.y);
        }

        uint32_t indexCount = 0;
        fscanf_s(file, "IndexCount: %d\n", &indexCount);
        mesh.indices.resize(indexCount);
        for (uint32_t i = 2; i < indexCount; i += 3)
        {
            fscanf_s(file, "%d %d %d\n",
                &mesh.indices[i - 2],
                &mesh.indices[i - 1],
                &mesh.indices[i]);
        }
    }
    fclose(file);
}

void ModelIO::SaveMaterial(std::filesystem::path filePath, const Model& material)
{
}

void ModelIO::LoadMaterial(std::filesystem::path filePath, Model& material)
{
}

