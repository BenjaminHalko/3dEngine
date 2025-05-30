#pragma once

namespace Engine::Graphics
{
    class Texture
    {
    public:
        static void UnbindPS(uint32_t slot);

        Texture() = default;
        ~Texture();

        // Delete Copy
        Texture(const Texture&) = delete;
        Texture& operator = (const Texture&) = delete;

        // Allow Move
        Texture(Texture&& rhs) noexcept;
        Texture& operator = (Texture&& rhs) noexcept;

        void Initialize(const std::filesystem::path& fileName);

        void Terminate();

        void BindVS(uint32_t slot) const;
        void BindPS(uint32_t slot) const;

        void* GetRawData() const;

    private:
        ID3D11ShaderResourceView* mShaderResourceView = nullptr;
    };
}