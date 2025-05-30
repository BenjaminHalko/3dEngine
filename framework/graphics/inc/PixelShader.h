#pragma once

namespace Engine::Graphics
{
	class PixelShader final
	{
	public:
		void Initialize(const std::filesystem::path& shaderPath);
		void Terminate();
		void Bind();

	private:
		ID3D11PixelShader* mPixelShader = nullptr;
	};
}