#pragma once

#include "Model.h"

namespace Engine::Graphics
{
    using ModelId = std::size_t;

    class ModelManager final
    {
    public:
        static void StaticInitialize(const std::filesystem::path& rootPath);
        static void StaticTerminate();
        static ModelManager* Get();

        ModelManager() = default;
        ~ModelManager() = default;

        ModelManager(const ModelManager&) = delete;
        ModelManager(const ModelManager&&) = delete;
        ModelManager& operator=(const ModelManager&) = delete;
        ModelManager& operator=(const ModelManager&&) = delete;

        void SetRootDirectory(const std::filesystem::path& rootPath);
        ModelId GetModelId(const std::filesystem::path& filePath);
        ModelId LoadModel(const std::filesystem::path& filePath);
        const Model* GetModel(ModelId id);

    private:
        using Inventory = std::map<ModelId, std::unique_ptr<Model>>;
        Inventory mInventory;

        std::filesystem::path mRootDirectory;
    };
}


