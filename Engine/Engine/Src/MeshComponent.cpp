#include "Precompiled.h"
#include "MeshComponent.h"
#include "SaveUtil.h"

using namespace Engine;

void MeshComponent::Deserialize(const rapidjson::Value& value)
{
    RenderObjectComponent::Deserialize(value);

    ASSERT(value.HasMember("Shape") || !mMeshModel.meshData.empty(), "MeshComponent: Either needs mesh data or has data already!");

    Graphics::Model::MeshData& meshData = value.HasMember("Shape") ? mMeshModel.meshData.emplace_back() : mMeshModel.meshData.back();
    Graphics::Model::MaterialData& matData = value.HasMember("Shape") ? mMeshModel.materialData.emplace_back() : mMeshModel.materialData.back();

    if (value.HasMember("Shape"))
    {
        const auto& shapeData = value["Shape"].GetObj();
        if (shapeData.HasMember("Type"))
        {
            std::string shapeType = shapeData["Type"].GetString();
            if (shapeType == "Sphere")
            {
                int slices = shapeData["Slices"].GetInt();
                int rings = shapeData["Rings"].GetInt();
                float radius = shapeData["Radius"].GetFloat();
                meshData.mesh = Graphics::MeshBuilder::CreateSphere(slices, rings, radius);
            }
            else if (shapeType == "Plane")
            {
                int rows = shapeData["Rows"].GetInt();
                int columns = shapeData["Columns"].GetInt();
                float spacing = shapeData["Spacing"].GetFloat();
                bool horizontal = true;
                SaveUtil::ReadBool("Vertical", horizontal, shapeData);
                meshData.mesh = Graphics::MeshBuilder::CreatePlane(rows, columns, spacing, horizontal);
            }
            else if (shapeType == "Cube")
            {
                float size = shapeData["Size"].GetFloat();
                meshData.mesh = Graphics::MeshBuilder::CreateCube(size);
            }
            else
            {
                ASSERT(false, "MeshComponent: Unrecognised Shape Type %s!", shapeType.c_str());
            }
        }
        else
        {
            ASSERT(false, "MeshComponent: Must specify a shape type!");
        }
    }
    if (value.HasMember("Material"))
    {
        const auto& materialData = value["Material"].GetObj();
        SaveUtil::ReadColor("Emissive", matData.material.emissive, materialData);
        SaveUtil::ReadColor("Ambient", matData.material.ambient, materialData);
        SaveUtil::ReadColor("Diffuse", matData.material.diffuse, materialData);
        SaveUtil::ReadColor("Specular", matData.material.specular, materialData);
        SaveUtil::ReadFloat("Shininess", matData.material.shininess, materialData);
    }
    if (value.HasMember("Textures"))
    {
        const auto& textureData = value["Textures"].GetObj();
        SaveUtil::ReadString("DiffuseMap", matData.diffuseMapName, textureData);
        SaveUtil::ReadString("NormalMap", matData.normalMapName, textureData);
        SaveUtil::ReadString("SpecMap", matData.specMapName, textureData);
        SaveUtil::ReadString("BumpMap", matData.bumpMapName, textureData);
    }
}

const Graphics::Model& MeshComponent::GetModel() const
{
    return mMeshModel;
}
