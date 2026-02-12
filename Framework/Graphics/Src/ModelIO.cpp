#include "Precompiled.h"
#include "ModelIO.h"
#include "Model.h"
#include "AnimationBuilder.h"

#ifndef MAX_PATH
#define MAX_PATH 4096
#endif

using namespace Engine;
using namespace Engine::Graphics;

void AnimationIO::Write(FILE* file, const Animation& animation)
{
    uint32_t keyCount = static_cast<uint32_t>(animation.mPositionKeys.size());
    fprintf_s(file, "PositionKeys: %d\n", keyCount);
    for (auto& key : animation.mPositionKeys)
    {
        fprintf_s(file, "%f %f %f %f\n", key.time, key.key.x, key.key.y, key.key.z);
    }
    keyCount = static_cast<uint32_t>(animation.mRotationKeys.size());
    fprintf_s(file, "RotationKeys: %d\n", keyCount);
    for (auto& key : animation.mRotationKeys)
    {
        fprintf_s(file, "%f %f %f %f %f\n", key.time, key.key.x, key.key.y, key.key.z, key.key.w);
    }
    keyCount = static_cast<uint32_t>(animation.mScaleKeys.size());
    fprintf_s(file, "ScaleKeys: %d\n", keyCount);
    for (auto& key : animation.mScaleKeys)
    {
        fprintf_s(file, "%f %f %f %f\n", key.time, key.key.x, key.key.y, key.key.z);
    }
}

void AnimationIO::Read(FILE* file, Animation& animation)
{
    AnimationBuilder builder;
    uint32_t keyCount = 0;
    float time = 0.0f;
    fscanf_s(file, "PositionKeys: %d\n", &keyCount);
    for (uint32_t k = 0; k < keyCount; ++k)
    {
        Math::Vector3 pos;
        fscanf_s(file, "%f %f %f %f\n", &time, &pos.x, &pos.y, &pos.z);
        builder.AddPositionKey(pos, time);
    }
    fscanf_s(file, "RotationKeys: %d\n", &keyCount);
    for (uint32_t k = 0; k < keyCount; ++k)
    {
        Math::Quaternion rot;
        fscanf_s(file, "%f %f %f %f %f\n", &time, &rot.x, &rot.y, &rot.z, &rot.w);
        builder.AddRotationKey(rot, time);
    }
    fscanf_s(file, "ScaleKeys: %d\n", &keyCount);
    for (uint32_t k = 0; k < keyCount; ++k)
    {
        Math::Vector3 scale;
        fscanf_s(file, "%f %f %f %f\n", &time, &scale.x, &scale.y, &scale.z);
        builder.AddScaleKey(scale, time);
    }
    animation = builder.Build();
}

void ModelIO::SaveModel(std::filesystem::path filePath, const Model& model)
{
    if (model.meshData.empty())
    {
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
            fprintf_s(file, "%f %f %f %f %f %f %f %f %f %f %f %d %d %d %d %f %f %f %f\n",
                v.position.x, v.position.y, v.position.z,
                v.normal.x, v.normal.y, v.normal.z,
                v.tangent.x, v.tangent.y, v.tangent.z,
                v.uvCoord.x, v.uvCoord.y,
                v.boneIndices[0], v.boneIndices[1], v.boneIndices[2], v.boneIndices[3],
                v.boneWeights[0], v.boneWeights[1], v.boneWeights[2], v.boneWeights[3]);
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
            fscanf_s(file, "%f %f %f %f %f %f %f %f %f %f %f %d %d %d %d %f %f %f %f\n",
                &v.position.x, &v.position.y, &v.position.z,
                &v.normal.x, &v.normal.y, &v.normal.z,
                &v.tangent.x, &v.tangent.y, &v.tangent.z,
                &v.uvCoord.x, &v.uvCoord.y,
                &v.boneIndices[0], &v.boneIndices[1], &v.boneIndices[2], &v.boneIndices[3],
                &v.boneWeights[0], &v.boneWeights[1], &v.boneWeights[2], &v.boneWeights[3]);
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

void ModelIO::SaveMaterial(std::filesystem::path filePath, const Model& model)
{
    if (model.materialData.empty())
    {
        return;
    }

    filePath.replace_extension("material");

    FILE* file = nullptr;

    fopen_s(&file, filePath.u8string().c_str(), "w");

    if (file == nullptr)
    {
        return;
    }

    uint32_t materialCount = static_cast<uint32_t>(model.materialData.size());

    fprintf_s(file, "MaterialCount: %d\n", materialCount);
    for (const Model::MaterialData& materialData : model.materialData)
    {
        const Material& m = materialData.material;
        fprintf_s(file, "%f %f %f %f\n", m.emissive.r, m.emissive.g, m.emissive.b, m.emissive.a);
        fprintf_s(file, "%f %f %f %f\n", m.ambient.r, m.ambient.g, m.ambient.b, m.ambient.a);
        fprintf_s(file, "%f %f %f %f\n", m.diffuse.r, m.diffuse.g, m.diffuse.b, m.diffuse.a);
        fprintf_s(file, "%f %f %f %f\n", m.specular.r, m.specular.g, m.specular.b, m.specular.a);
        fprintf_s(file, "Shininess: %f\n", m.shininess);

        fprintf_s(file, "%s\n", materialData.diffuseMapName.empty() ? "<NONE>" : materialData.diffuseMapName.c_str());
        fprintf_s(file, "%s\n", materialData.specMapName.empty() ? "<NONE>" : materialData.specMapName.c_str());
        fprintf_s(file, "%s\n", materialData.normalMapName.empty() ? "<NONE>" : materialData.normalMapName.c_str());
        fprintf_s(file, "%s\n", materialData.bumpMapName.empty() ? "<NONE>" : materialData.bumpMapName.c_str());
    }

    fclose(file);
}

void ModelIO::LoadMaterial(std::filesystem::path filePath, Model& model)
{
    filePath.replace_extension("material");

    FILE* file = nullptr;

    fopen_s(&file, filePath.u8string().c_str(), "r");

    if (file == nullptr)
    {
        return;
    }

    auto TryReadTextureName = [&](auto& fileName)
        {
            char buffer[MAX_PATH];
#ifdef _WIN32
            fscanf_s(file, "%s\n", buffer, (uint32_t)sizeof(buffer));
#else
            fscanf(file, "%s\n", buffer);
#endif
            if (strcmp(buffer, "<NONE>") != 0)
            {
                fileName = filePath.replace_filename(buffer).string();
            }
        };

    uint32_t materialCount = 0;

    fscanf_s(file, "MaterialCount: %d\n", &materialCount);
    model.materialData.resize(materialCount);
    for (Model::MaterialData& materialData : model.materialData)
    {
        Material& m = materialData.material;
        fscanf_s(file, "%f %f %f %f\n", &m.emissive.r, &m.emissive.g, &m.emissive.b, &m.emissive.a);
        fscanf_s(file, "%f %f %f %f\n", &m.ambient.r, &m.ambient.g, &m.ambient.b, &m.ambient.a);
        fscanf_s(file, "%f %f %f %f\n", &m.diffuse.r, &m.diffuse.g, &m.diffuse.b, &m.diffuse.a);
        fscanf_s(file, "%f %f %f %f\n", &m.specular.r, &m.specular.g, &m.specular.b, &m.specular.a);
        fscanf_s(file, "Shininess: %f\n", &m.shininess);

        TryReadTextureName(materialData.diffuseMapName);
        TryReadTextureName(materialData.specMapName);
        TryReadTextureName(materialData.normalMapName);
        TryReadTextureName(materialData.bumpMapName);
    }

    fclose(file);
}

void ModelIO::SaveSkeleton(std::filesystem::path filePath, Model& model)
{
    if (model.skeleton == nullptr || model.skeleton->bones.empty())
    {
        return;
    }
    filePath.replace_extension("skeleton");
    FILE* file = nullptr;
    fopen_s(&file, filePath.u8string().c_str(), "w");
    if (file == nullptr)
    {
        return;
    }

    auto WriteMatrix = [&file](auto& m)
    {
        fprintf_s(file, "%f %f %f %f\n", m._11, m._12, m._13, m._14);
        fprintf_s(file, "%f %f %f %f\n", m._21, m._22, m._23, m._24);
        fprintf_s(file, "%f %f %f %f\n", m._31, m._32, m._33, m._34);
        fprintf_s(file, "%f %f %f %f\n", m._41, m._42, m._43, m._44);
    };

    uint32_t boneCount = static_cast<uint32_t>(model.skeleton->bones.size());
    fprintf_s(file, "BoneCount: %d\n", boneCount);
    fprintf_s(file, "RootBone: %d\n", model.skeleton->root->index);
    for (uint32_t i = 0; i < boneCount; ++i)
    {
        const Bone* boneData = model.skeleton->bones[i].get();
        fprintf_s(file, "BoneName: %s\n", boneData->name.c_str());
        fprintf_s(file, "BoneIndex: %d\n", boneData->index);
        fprintf_s(file, "ParentIndex: %d\n", boneData->parentIndex);

        uint32_t childCount = static_cast<uint32_t>(boneData->childrenIndices.size());
        fprintf_s(file, "ChildCount: %d\n", childCount);
        for (uint32_t c = 0; c < childCount; ++c)
        {
            fprintf_s(file, "%d\n", boneData->childrenIndices[c]);
        }

        WriteMatrix(boneData->toParentTransform);
        WriteMatrix(boneData->offsetTransform);
    }
    fclose(file);
}

void ModelIO::LoadSkeleton(std::filesystem::path filePath, Model& model)
{
    filePath.replace_extension("skeleton");
    FILE* file = nullptr;
    fopen_s(&file, filePath.u8string().c_str(), "r");
    if (file == nullptr)
    {
        return;
    }

    auto ReadMatrix = [&file](auto& m)
        {
            fscanf_s(file, "%f %f %f %f\n", &m._11, &m._12, &m._13, &m._14);
            fscanf_s(file, "%f %f %f %f\n", &m._21, &m._22, &m._23, &m._24);
            fscanf_s(file, "%f %f %f %f\n", &m._31, &m._32, &m._33, &m._34);
            fscanf_s(file, "%f %f %f %f\n", &m._41, &m._42, &m._43, &m._44);
        };

    model.skeleton = std::make_unique<Skeleton>();
    uint32_t boneCount = 0;
    uint32_t rootIndex = 0;
    fscanf_s(file, "BoneCount: %d\n", &boneCount);
    fscanf_s(file, "RootBone: %d\n", &rootIndex);
    model.skeleton->bones.resize(boneCount);
    for (uint32_t i = 0; i < boneCount; ++i)
    {
        model.skeleton->bones[i] = std::make_unique<Bone>();
    }
    model.skeleton->root = model.skeleton->bones[rootIndex].get();

    for (uint32_t i = 0; i < boneCount; ++i)
    {
        Bone* boneData = model.skeleton->bones[i].get();
        char nameBuffer[MAX_PATH];
#ifdef _WIN32
        fscanf_s(file, "BoneName: %s\n", nameBuffer, (uint32_t)sizeof(nameBuffer));
#else
        fscanf(file, "BoneName: %s\n", nameBuffer);
#endif
        fscanf_s(file, "BoneIndex: %d\n", &boneData->index);
        fscanf_s(file, "ParentIndex: %d\n", &boneData->parentIndex);
        boneData->name = std::move(nameBuffer);
        boneData->parent = (boneData->parentIndex >= 0) ? model.skeleton->bones[boneData->parentIndex].get() : nullptr;

        uint32_t childCount = 0;
        fscanf_s(file, "ChildCount: %d\n", &childCount);
        boneData->childrenIndices.resize(childCount);
        boneData->children.resize(childCount);
        for (uint32_t c = 0; c < childCount; ++c)
        {
            uint32_t childIndex = 0;
            fscanf_s(file, "%d\n", &childIndex);
            boneData->childrenIndices[c] = childIndex;
            boneData->children[c] = model.skeleton->bones[childIndex].get();
        }

        ReadMatrix(boneData->toParentTransform);
        ReadMatrix(boneData->offsetTransform);
    }
    fclose(file);
}

void ModelIO::SaveAnimation(std::filesystem::path filePath, Model& model)
{
    if (model.skeleton == nullptr || model.skeleton->bones.empty() || model.animationClips.empty())
    {
        return;
    }

    filePath.replace_extension("animset");

    FILE* file = nullptr;
    fopen_s(&file, filePath.u8string().c_str(), "w");
    if (file == nullptr)
    {
        return;
    }

    uint32_t animClipCount = static_cast<uint32_t>(model.animationClips.size());
    fprintf_s(file, "AnimClipCount: %d\n", animClipCount);
    for (uint32_t i = 0; i < animClipCount; ++i)
    {
        const AnimationClip& animClipData = model.animationClips[i];
        fprintf_s(file, "AnimClipName: %s\n", animClipData.name.c_str());
        fprintf_s(file, "TickDuration: %f\n", animClipData.tickDuration);
        fprintf_s(file, "TicksPerSecond: %f\n", animClipData.ticksPerSecond);

        uint32_t boneAnimCount = static_cast<uint32_t>(animClipData.boneAnimations.size());
        fprintf_s(file, "BoneAnimCount: %d\n", boneAnimCount);
        for (uint32_t b = 0; b < boneAnimCount; ++b)
        {
            const Animation* boneAnim = animClipData.boneAnimations[b].get();
            if (boneAnim == nullptr)
            {
                fprintf_s(file, "<NONE>\n");
                continue;
            }
            fprintf_s(file, "<ANIMATION>\n");
            AnimationIO::Write(file, *boneAnim);
        }
    }
    fclose(file);
}

void ModelIO::LoadAnimation(std::filesystem::path filePath, Model& model)
{
    filePath.replace_extension("animset");

    FILE* file = nullptr;
    fopen_s(&file, filePath.u8string().c_str(), "r");
    if (file == nullptr)
    {
        return;
    }

    uint32_t animClipCount = 0;
    fscanf_s(file, "AnimClipCount: %d\n", &animClipCount);
    for (uint32_t i = 0; i < animClipCount; ++i)
    {
        AnimationClip& animClipData = model.animationClips.emplace_back();
        char animClipName[MAX_PATH];
#ifdef _WIN32
        fscanf_s(file, "AnimClipName: %s\n", animClipName, (uint32_t)sizeof(animClipName));
#else
        fscanf(file, "AnimClipName: %s\n", animClipName);
#endif
        fscanf_s(file, "TickDuration: %f\n", &animClipData.tickDuration);
        fscanf_s(file, "TicksPerSecond: %f\n", &animClipData.ticksPerSecond);
        animClipData.name = std::move(animClipName);

        uint32_t boneAnimCount = 0;
        fscanf_s(file, "BoneAnimCount: %d\n", &boneAnimCount);
        animClipData.boneAnimations.resize(boneAnimCount);
        for (uint32_t b = 0; b < boneAnimCount; ++b)
        {
            char tag[128]{};
#ifdef _WIN32
            fscanf_s(file, "%s\n", tag, (uint32_t)sizeof(tag));
#else
            fscanf(file, "%s\n", tag);
#endif
            if (strcmp(tag, "<ANIMATION>") == 0)
            {
                animClipData.boneAnimations[b] = std::make_unique<Animation>();
                AnimationIO::Read(file, *animClipData.boneAnimations[b]);
            }
        }
    }
    fclose(file);
}
