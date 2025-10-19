#pragma once

#include "Texture.h"

namespace Engine::Graphics
{
	using TextureId = std::size_t;

	class TextureManager final
	{
	public:
		static void StaticInitialize(const std::filesystem::path& root);
		static void StaticTerminate();
		static TextureManager* Get();

		TextureManager() = default;
		~TextureManager();

		TextureManager(const TextureManager&) = delete;
		TextureManager(const TextureManager&&) = delete;
		TextureManager& operator=(const TextureManager&) = delete;
		TextureManager& operator=(const TextureManager&&) = delete;

		void SetRootDirectory(const std::filesystem::path& root);
		TextureId LoadTexture(const std::filesystem::path& filename, bool useRootDir = true);
		const Texture* GetTexture(TextureId id);
		void ReleaseTexture(TextureId id);

		void BindVS(TextureId id, uint32_t slot) const;
		void BindPS(TextureId id, uint32_t slot) const;

	private:
		struct Entry
		{
			std::unique_ptr<Texture> texture;
			uint32_t refCount = 0;
		};
		using Inventory = std::unordered_map<TextureId, Entry>;
		Inventory mInventory;
		std::filesystem::path mRootDirectory;
	};
}
