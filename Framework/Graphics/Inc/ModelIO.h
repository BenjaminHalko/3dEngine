#pragma once

namespace Engine::Graphics
{
    struct Model;
    class Animation;

    class AnimationIO
    {
    public:
        static void Write(FILE* file, const Animation& animation);
        static void Read(FILE* file, Animation& animation);
    };

    namespace ModelIO
    {
        void SaveModel(std::filesystem::path filePath, const Model& model);
        void LoadModel(std::filesystem::path filePath, Model& model);

        void SaveMaterial(std::filesystem::path filePath, const Model& material);
        void LoadMaterial(std::filesystem::path filePath, Model& material);

        void SaveSkeleton(std::filesystem::path filePath, Model& model);
        void LoadSkeleton(std::filesystem::path filePath, Model& model);

        void SaveAnimation(std::filesystem::path filePath, Model& model);
        void LoadAnimation(std::filesystem::path filePath, Model& model);
    }
}
