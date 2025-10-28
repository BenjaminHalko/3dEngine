#include "Precompiled.h"
#include "RenderObject.h"

using namespace Engine;
using namespace Engine::Graphics;

void RenderObject::Terminate()
{
    meshBuffer.Terminate();
    TextureManager* tm = TextureManager::Get();
    tm->ReleaseTexture(diffuseMapId);
    tm->ReleaseTexture(specMapId);
    tm->ReleaseTexture(normalMapId);
    tm->ReleaseTexture(bumpMapId);
}

void RenderGroup::Initialize(const std::filesystem::path& modelFilePath)
{
    modelId = ModelManager::Get()->LoadModel(modelFilePath);
    const Model* model = ModelManager::Get()->GetModel(modelId);
    ASSERT(model != nullptr, "RenderGroup: Failed to load %s", modelFilePath.u8string().c_str());

    for (const Model::MeshData& meshData : model->meshData)
    {
        RenderObject& renderObject = renderObjects.emplace_back();
        renderObject.meshBuffer.Initialize(meshData.mesh);
        if (meshData.materialIndex < model->materialData.size())
        {
            // Add Material Data
            
        }
    }
}

void RenderGroup::Terminate()
{
    for (RenderObject& renderObject : renderObjects)
    {
        renderObject.Terminate();
    }
    renderObjects.clear();
}


