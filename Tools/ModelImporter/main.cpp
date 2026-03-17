#include <Engine/Inc/Engine.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <cstdio>

#ifndef MAX_PATH
#define MAX_PATH 4096
#endif

using namespace Engine;
using namespace Engine::Graphics;
using namespace Engine::Math;

using BoneIndexMap = std::map<std::string, uint32_t>;

struct Arguments
{
    std::filesystem::path inputFileName;
    std::filesystem::path outputFileName;
    float scale = 1.0f;    // 1 Unit = 1 Millimeter
    bool animOnly = false; // Export only animation data
};

std::optional<Arguments> ParseArgs(int argc, char* argv[])
{
    if (argc < 3)
    {
        printf("Usage: ModelImporter <input file> <output file> [scale]\n");
        return std::nullopt;
    }

    // .. .. .. -scale 0.1 <inputFileName> <outputFileName>
    Arguments args;
    args.inputFileName = argv[argc - 2];
    args.outputFileName = argv[argc - 1];
    for (int i = 0; i + 2 < argc; ++i)
    {
        if (strcmp(argv[i], "-scale") == 0)
        {
            args.scale = atof(argv[i + 1]);
            ++i;
        }
        // Check to see if we set -animOnly flag to 1 in our command line arguments
        else if (strcmp(argv[i], "-animOnly") == 0)
        {
            // If true, only save the animation data
            args.animOnly = atoi(argv[i + 1]) == 1;
            ++i;
        }
    }
    return args;
}

Vector3 ToVector3(const aiVector3D& v)
{
    return {static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z)};
}

Vector2 ToTexCoord(const aiVector3D& v)
{
    return {static_cast<float>(v.x), static_cast<float>(v.y)};
}

Color ToColor(const aiColor3D& c)
{
    return {static_cast<float>(c.r),
            static_cast<float>(c.g),
            static_cast<float>(c.b),
            static_cast<float>(1.0f)};
}

Quaternion ToQuaternion(const aiQuaternion& q)
{
    return {static_cast<float>(q.x),
            static_cast<float>(q.y),
            static_cast<float>(q.z),
            static_cast<float>(q.w)};
}

Matrix4 ToMatrix4(const aiMatrix4x4& m)
{
    return {static_cast<float>(m.a1),
            static_cast<float>(m.b1),
            static_cast<float>(m.c1),
            static_cast<float>(m.d1),
            static_cast<float>(m.a2),
            static_cast<float>(m.b2),
            static_cast<float>(m.c2),
            static_cast<float>(m.d2),
            static_cast<float>(m.a3),
            static_cast<float>(m.b3),
            static_cast<float>(m.c3),
            static_cast<float>(m.d3),
            static_cast<float>(m.a4),
            static_cast<float>(m.b4),
            static_cast<float>(m.c4),
            static_cast<float>(m.d4)};
}

void ExportEmbeddedTexture(const aiTexture* texture,
                           const Arguments& args,
                           const std::filesystem::path& fileName)
{
    printf("Exporting Embedded Texture: %s\n", fileName.u8string().c_str());

    std::string fullFileName = args.outputFileName.u8string();

    fullFileName = fullFileName.substr(0, fullFileName.rfind('/') + 1);
    fullFileName += fileName.filename().u8string();

    FILE* file = nullptr;
    auto err = fopen_s(&file, fullFileName.c_str(), "wb");
    if (err != 0 || file == nullptr)
    {
        printf("Error: Failed to create texture file: %s\n", fullFileName.c_str());
        return;
    }

    size_t written = fwrite(texture->pcData, 1, texture->mWidth, file);
    ASSERT(written == texture->mWidth, "Error: Failed to extract embedded texture data!");
    fclose(file);
}

std::string FindTexture(const aiScene* scene,
                        const aiMaterial* aiMaterial,
                        aiTextureType textureType,
                        const Arguments& args,
                        const std::string& suffix,
                        uint32_t materealIndex)
{
    const uint32_t textureCount = aiMaterial->GetTextureCount(textureType);
    if (textureCount == 0)
    {
        return "";
    }

    std::filesystem::path textureName;
    aiString texturePath;
    if (aiMaterial->GetTexture(textureType, 0, &texturePath) == aiReturn_SUCCESS)
    {
        if (texturePath.C_Str()[0] == '*')
        {
            std::string fileName = args.inputFileName.u8string();
            fileName.erase(fileName.length() - 4); // remove .fbx
            fileName += suffix;
            fileName += texturePath.C_Str()[1]; // get the index number

            ASSERT(scene->HasTextures(), "Error: Model has no embedded textures!");

            int textureIndex = atoi(texturePath.C_Str() + 1);
            ASSERT(textureIndex < static_cast<int>(scene->mNumTextures),
                   "Error: Texture index out of bounds!");

            const aiTexture* embeddedTexture = scene->mTextures[textureIndex];
            ASSERT(embeddedTexture->mHeight == 0, "Error: Compressed texture expected!");

            if (embeddedTexture->CheckFormat("jpg"))
            {
                fileName += ".jpg";
            }
            else if (embeddedTexture->CheckFormat("png"))
            {
                fileName += ".png";
            }
            else
            {
                ASSERT(false, "Error: Unsupported texture format!");
            }

            printf("Adding Texture: %s\n", fileName.c_str());
            ExportEmbeddedTexture(embeddedTexture, args, fileName);
            textureName = fileName;
        }
        else if (auto embeddedTexture = scene->GetEmbeddedTexture(texturePath.C_Str());
                 embeddedTexture)
        {
            std::filesystem::path embeddedFilePath = texturePath.C_Str();
            std::string fileName = args.inputFileName.u8string();
            fileName.erase(fileName.length() - 4); // remove .fbx
            fileName += suffix;
            fileName += "_" + std::to_string(materealIndex);
            fileName += embeddedFilePath.extension().u8string();

            printf("Adding Texture: %s\n", fileName.c_str());
            ExportEmbeddedTexture(embeddedTexture, args, fileName);
            textureName = fileName;
        }
        else
        {
            std::filesystem::path filePath = texturePath.C_Str();
            std::string fileName = filePath.u8string();

            printf("Adding Texture: %s\n", fileName.c_str());
            textureName = fileName;
        }
    }

    return textureName.filename().u8string();
}

Bone* BuildSkeleton(const aiNode* sceneNode,
                    Bone* parent,
                    Skeleton& skeleton,
                    BoneIndexMap& boneIndexMap)
{
    Bone* bone = nullptr;
    std::string boneName = sceneNode->mName.C_Str();
    auto iter = boneIndexMap.find(boneName);
    if (iter != boneIndexMap.end())
    {
        // Bone already exists:
        bone = skeleton.bones[iter->second].get();
    }
    else
    {
        // Create new Bone:
        bone = skeleton.bones.emplace_back(std::make_unique<Bone>()).get();
        bone->index = static_cast<int>(skeleton.bones.size()) - 1;
        bone->offsetTransform = Matrix4::Identity;
        if (boneName.empty())
        {
            bone->name = "NoName" + std::to_string(bone->index);
        }
        else
        {
            bone->name = std::move(boneName);
        }
        boneIndexMap.emplace(bone->name, bone->index);
    }
    if (skeleton.root == nullptr && parent == nullptr)
    {
        skeleton.root = bone;
    }

    bone->parent = parent;
    bone->parentIndex = (parent) ? parent->index : -1;
    bone->toParentTransform = ToMatrix4(sceneNode->mTransformation);

    bone->children.reserve(sceneNode->mNumChildren);
    for (uint32_t childIndex = 0; childIndex < sceneNode->mNumChildren; ++childIndex)
    {
        Bone* child = BuildSkeleton(sceneNode->mChildren[childIndex], bone, skeleton, boneIndexMap);
        bone->children.push_back(child);
        bone->childrenIndices.push_back(child->index);
    }
    return bone;
}

// Helper Function to get Indices of Bones
uint32_t GetBoneIndex(const aiBone* nodeBone, const BoneIndexMap& boneIndexMap)
{
    std::string boneName = nodeBone->mName.C_Str();
    ASSERT(!boneName.empty(), "Error: Bone has no name!");

    auto iter = boneIndexMap.find(boneName);
    ASSERT(iter != boneIndexMap.end(), "Error: aiBone not found in Bone Index Map!");
    return iter->second;
}

void SetBoneOffsetTransform(const aiBone* nodeBone,
                            Skeleton& skeleton,
                            const BoneIndexMap& boneIndexMap)
{
    uint32_t boneIndex = GetBoneIndex(nodeBone, boneIndexMap);
    Bone* bone = skeleton.bones[boneIndex].get();
    bone->offsetTransform = ToMatrix4(nodeBone->mOffsetMatrix);
}

int main(int argc, char* argv[])
{
    const auto argsOpt = ParseArgs(argc, argv);
    if (!argsOpt.has_value())
    {
        printf("Not enough arguments, import fbx failed!\n");
        return -1;
    }

    printf("Begin Import\n");
    const Arguments args = argsOpt.value();

    // Using Assimp to load model:
    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

    const uint32_t flags = aiProcessPreset_TargetRealtime_Quality | aiProcess_ConvertToLeftHanded |
                           aiProcess_PopulateArmatureData;
    const aiScene* scene = importer.ReadFile(args.inputFileName.u8string().c_str(), flags);

    if (scene == nullptr)
    {
        printf("Failed to load model file: %s\n", importer.GetErrorString());
        return -1;
    }

    printf("Importing model located at: %s\n", args.inputFileName.u8string().c_str());

    Model model;
    BoneIndexMap boneIndexMap;

    if (scene->HasMeshes())
    {
        printf("Building Skeleton...\n");
        model.skeleton = std::make_unique<Skeleton>();
        // Build Skeleton:
        BuildSkeleton(scene->mRootNode, nullptr, *model.skeleton, boneIndexMap);

        // Applies bone offset transforms (skinning mesh data offsets)
        for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
        {
            const auto& aiMesh = scene->mMeshes[meshIndex];
            if (aiMesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
            {
                continue;
            }
            if (aiMesh->HasBones())
            {
                for (uint32_t b = 0; b < aiMesh->mNumBones; ++b)
                {
                    const auto& aiBone = aiMesh->mBones[b];
                    SetBoneOffsetTransform(aiBone, *model.skeleton, boneIndexMap);
                }
            }
        }
        // This offsets the bones transforms by the import scale
        for (auto& bone : model.skeleton->bones)
        {
            bone->offsetTransform._41 *= args.scale;
            bone->offsetTransform._42 *= args.scale;
            bone->offsetTransform._43 *= args.scale;
            bone->toParentTransform._41 *= args.scale;
            bone->toParentTransform._42 *= args.scale;
            bone->toParentTransform._43 *= args.scale;
        }

        if (!args.animOnly)
        {
            printf("Reading Mesh Data...\n");

            for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
            {
                const auto& aiMesh = scene->mMeshes[meshIndex];
                if (aiMesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
                {
                    printf("Skipping non-triangle mesh: %s\n", aiMesh->mName.C_Str());
                    continue;
                }

                const uint32_t numVertices = aiMesh->mNumVertices;
                const uint32_t numFaces = aiMesh->mNumFaces;
                const uint32_t numIndices = numFaces * 3;

                Model::MeshData& meshData = model.meshData.emplace_back();

                printf("Reading Material Index for Mesh...\n");
                meshData.materialIndex = aiMesh->mMaterialIndex;

                printf("Reading Vertices for Mesh...\n");
                Mesh& mesh = meshData.mesh;
                mesh.vertices.reserve(numVertices);

                const aiVector3D* positions = aiMesh->mVertices;
                const aiVector3D* normals = aiMesh->mNormals;
                const aiVector3D* tangents =
                    (aiMesh->HasTangentsAndBitangents()) ? aiMesh->mTangents : nullptr;
                const aiVector3D* texCoords =
                    (aiMesh->HasTextureCoords(0)) ? aiMesh->mTextureCoords[0] : nullptr;

                for (uint32_t v = 0; v < numVertices; ++v)
                {
                    Vertex& vertex = mesh.vertices.emplace_back();
                    vertex.position = ToVector3(positions[v]) * args.scale;
                    vertex.normal = ToVector3(normals[v]);
                    vertex.tangent = (tangents) ? ToVector3(tangents[v]) : Vector3::Zero;
                    vertex.uvCoord = (texCoords) ? ToTexCoord(texCoords[v]) : Vector2::Zero;
                }

                printf("Reading Indices for Mesh...\n");
                mesh.indices.reserve(numIndices);
                const auto& aiFaces = aiMesh->mFaces;
                for (uint32_t f = 0; f < numFaces; ++f)
                {
                    const auto& aiFace = aiFaces[f];
                    for (uint32_t i = 0; i < 3; ++i)
                    {
                        mesh.indices.push_back(aiFace.mIndices[i]);
                    }
                }
                // Get bone weights from mesh bones and apply to vertex data
                if (aiMesh->HasBones())
                {
                    printf("Reading Bone Weights for Mesh...\n");
                    std::vector<int> numWeightsAdded(mesh.vertices.size());
                    for (uint32_t b = 0; b < aiMesh->mNumBones; ++b)
                    {
                        const auto& aiBone = aiMesh->mBones[b];
                        uint32_t boneIndex = GetBoneIndex(aiBone, boneIndexMap);
                        for (uint32_t w = 0; w < aiBone->mNumWeights; ++w)
                        {
                            const aiVertexWeight& weight = aiBone->mWeights[w];
                            // Get the vertex that the bone is influencing
                            Vertex& vertex = mesh.vertices[weight.mVertexId];
                            // Get the number of bones already affecting this vertex
                            int& weightCount = numWeightsAdded[weight.mVertexId];
                            if (weightCount < Vertex::MaxBoneWeights)
                            {
                                vertex.boneIndices[weightCount] = boneIndex;
                                vertex.boneWeights[weightCount] = weight.mWeight;
                                ++weightCount;
                            }
                        }
                    }
                }
            }
        }
    }

    if (!args.animOnly && scene->HasMaterials())
    {
        printf("Reading Material Data...\n");

        const uint32_t numMaterials = scene->mNumMaterials;
        model.materialData.reserve(numMaterials);

        for (uint32_t materialIndex = 0; materialIndex < numMaterials; ++materialIndex)
        {
            const auto& aiMaterial = scene->mMaterials[materialIndex];
            aiColor3D emissive, ambient, diffuse, specular;
            ai_real specularPower = 10.0f;

            aiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, emissive);
            aiMaterial->Get(AI_MATKEY_COLOR_AMBIENT, ambient);
            aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
            aiMaterial->Get(AI_MATKEY_COLOR_SPECULAR, specular);
            aiMaterial->Get(AI_MATKEY_SHININESS, specularPower);

            Model::MaterialData& materialData = model.materialData.emplace_back();
            materialData.material.emissive = ToColor(emissive);
            materialData.material.ambient = ToColor(ambient);
            materialData.material.diffuse = ToColor(diffuse);
            materialData.material.specular = ToColor(specular);
            materialData.material.shininess = static_cast<float>(specularPower);

            materialData.diffuseMapName =
                FindTexture(scene, aiMaterial, aiTextureType_DIFFUSE, args, "_diff", materialIndex);
            materialData.specMapName = FindTexture(
                scene, aiMaterial, aiTextureType_SPECULAR, args, "_spec", materialIndex);
            materialData.normalMapName =
                FindTexture(scene, aiMaterial, aiTextureType_NORMALS, args, "_norm", materialIndex);
            materialData.bumpMapName = FindTexture(
                scene, aiMaterial, aiTextureType_DISPLACEMENT, args, "_bump", materialIndex);
        }
    }

    // Read animations from assimp, then store in the model
    if (scene->HasAnimations())
    {
        printf("Building Animations...\n");

        for (uint32_t animIndex = 0; animIndex < scene->mNumAnimations; ++animIndex)
        {
            const auto& aiAnim = scene->mAnimations[animIndex];

            // Emplace_back, adds to the vector and returns last item
            AnimationClip& animClip = model.animationClips.emplace_back();
            if (aiAnim->mName.length > 0)
            {
                animClip.name = aiAnim->mName.C_Str();
            }
            else
            {
                animClip.name = "Anim" + std::to_string(animIndex);
            }
            animClip.tickDuration = static_cast<float>(aiAnim->mDuration);
            animClip.ticksPerSecond =
                static_cast<float>(aiAnim->mTicksPerSecond); // Frame Rate of Animation

            printf("Reading Bone Animations for %s...\n", animClip.name.c_str());

            animClip.boneAnimations.resize(model.skeleton->bones.size());
            for (uint32_t boneAnimIndex = 0; boneAnimIndex < aiAnim->mNumChannels; ++boneAnimIndex)
            {
                const auto& aiBoneAnim = aiAnim->mChannels[boneAnimIndex];
                const int boneIndex = boneIndexMap[aiBoneAnim->mNodeName.C_Str()];
                auto& boneAnimation = animClip.boneAnimations[boneIndex];
                boneAnimation = std::make_unique<Animation>();

                AnimationBuilder builder;
                for (uint32_t keyIndex = 0; keyIndex < aiBoneAnim->mNumPositionKeys; ++keyIndex)
                {
                    const aiVectorKey& pos = aiBoneAnim->mPositionKeys[keyIndex];
                    builder.AddPositionKey(ToVector3(pos.mValue) * args.scale,
                                           static_cast<float>(pos.mTime));
                }
                for (uint32_t keyIndex = 0; keyIndex < aiBoneAnim->mNumRotationKeys; ++keyIndex)
                {
                    const aiQuatKey& rot = aiBoneAnim->mRotationKeys[keyIndex];
                    builder.AddRotationKey(ToQuaternion(rot.mValue), static_cast<float>(rot.mTime));
                }
                for (uint32_t keyIndex = 0; keyIndex < aiBoneAnim->mNumScalingKeys; ++keyIndex)
                {
                    const aiVectorKey& scale = aiBoneAnim->mScalingKeys[keyIndex];
                    builder.AddScaleKey(ToVector3(scale.mValue), static_cast<float>(scale.mTime));
                }
                *boneAnimation = builder.Build();
            }
        }
    }

    if (!args.animOnly)
    {
        printf("Saving Model...\n");
        ModelIO::SaveModel(args.outputFileName, model);

        printf("Saving Material...\n");
        ModelIO::SaveMaterial(args.outputFileName, model);

        printf("Saving Skeleton...\n");
        ModelIO::SaveSkeleton(args.outputFileName, model);
    }

    printf("Saving Animation...\n");
    ModelIO::SaveAnimation(args.outputFileName, model);

    printf("Import Complete!\n");

    return 0;
}
