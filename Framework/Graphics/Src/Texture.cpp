#include "Precompiled.h"
#include "Texture.h"
#include "GraphicsSystem.h"

// Use stb_image for cross-platform image loading (header-only)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_TGA
#include <stb/stb_image.h>

using namespace Engine;
using namespace Engine::Graphics;

// Helper: Load image using stb_image
namespace
{
struct ImageData
{
    unsigned char* pixels = nullptr;
    int width = 0;
    int height = 0;
    int channels = 0;
};

ImageData LoadImageFile(const std::filesystem::path& fileName)
{
    // Convert to UTF-8 string
    std::string fileStr = fileName.string();
    
    ImageData data;
    data.pixels = stbi_load(fileStr.c_str(), &data.width, &data.height, &data.channels, 4);  // Force RGBA
    data.channels = 4;  // stbi_load with last param 4 forces RGBA
    
    return data;
}

void FreeImageData(ImageData& data)
{
    if (data.pixels)
    {
        stbi_image_free(data.pixels);
        data.pixels = nullptr;
    }
}
}

void Texture::UnbindPS(uint32_t slot)
{
    static ID3D11ShaderResourceView* dummy = nullptr;
    GraphicsSystem::Get()->GetContext()->HSSetShaderResources(slot, 1, &dummy);
}

Texture::~Texture()
{
    ASSERT(mShaderResourceView == nullptr, "Texture: Terminate must be called");
}

Texture::Texture(Texture&& rhs) noexcept
    : mShaderResourceView(rhs.mShaderResourceView)
{
    rhs.mShaderResourceView = nullptr;
}

Texture& Texture::operator=(Texture&& rhs) noexcept
{
    mShaderResourceView = rhs.mShaderResourceView;
    rhs.mShaderResourceView = nullptr;
    return *this;
}

void Texture::Initialize(const std::filesystem::path& fileName)
{
    auto device = GraphicsSystem::Get()->GetDevice();
    
    // Load image data using stb_image
    ImageData imageData = LoadImageFile(fileName);
    
    if (!imageData.pixels)
    {
        ASSERT(false, "Texture: Failed to load image file %ls", fileName.c_str());
        return;
    }
    
    // Create D3D11 texture from image data
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = imageData.width;
    textureDesc.Height = imageData.height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;
    
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = imageData.pixels;
    initData.SysMemPitch = imageData.width * 4;  // 4 bytes per pixel (RGBA)
    initData.SysMemSlicePitch = 0;
    
    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = device->CreateTexture2D(&textureDesc, &initData, &texture);
    
    if (FAILED(hr))
    {
        FreeImageData(imageData);
        ASSERT(false, "Texture: Failed to create D3D11 texture from %ls", fileName.c_str());
        return;
    }
    
    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    
    hr = device->CreateShaderResourceView(texture, &srvDesc, &mShaderResourceView);
    
    SafeRelease(texture);
    FreeImageData(imageData);
    
    ASSERT(SUCCEEDED(hr), "Texture: Failed to create shader resource view for %ls", fileName.c_str());
}

void Texture::Terminate()
{
    SafeRelease(mShaderResourceView);
}

void Texture::BindVS(uint32_t slot) const
{
    auto context = GraphicsSystem::Get()->GetContext();
    context->VSSetShaderResources(slot, 1, &mShaderResourceView);
}

void Texture::BindPS(uint32_t slot) const
{
    auto context = GraphicsSystem::Get()->GetContext();
    context->PSSetShaderResources(slot, 1, &mShaderResourceView);
}

void* Texture::GetRawData() const
{
    return mShaderResourceView;
}
