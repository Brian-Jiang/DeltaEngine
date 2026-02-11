#include "Core/DTexture.h"

#include <DirectXTex.h>
#include <filesystem>

#include "IO/IOManager.h"
#include "Importers/TextureImporter.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/DXUtils.h"

using namespace DeltaEngine;
using namespace DirectX;

DeltaEngine::DTexture::DTexture()
    : m_sourcePath(L"")
    , m_sRGB(false)
{
}

DTexture::DTexture(const std::wstring& filePath, bool sRGB)
    : m_sourcePath(filePath)
    , m_sRGB(sRGB)
{
    LoadTexture();
}

DeltaEngine::DTexture::~DTexture()
{
    
}

inline UINT DeltaEngine::DTexture::GetWidth() const { return m_metadata->width; }

inline UINT DeltaEngine::DTexture::GetHeight() const { return m_metadata->height; }

inline DXGI_FORMAT DeltaEngine::DTexture::GetFormat() const { return m_metadata->format; }

void DeltaEngine::DTexture::LoadTexture()
{
    std::filesystem::path filePath(m_sourcePath);
    if (!std::filesystem::exists(filePath))
    {
        throw std::exception("File not found.");
    }

    if (filePath.extension() == ".dds") {
        ThrowIfFailed(LoadFromDDSFile(m_sourcePath.c_str(), DDS_FLAGS_FORCE_RGB, m_metadata.get(), *m_scratchImage));
    } else if (filePath.extension() == ".hdr") {
        ThrowIfFailed(LoadFromHDRFile(m_sourcePath.c_str(), m_metadata.get(), *m_scratchImage));
    } else if (filePath.extension() == ".tga") {
        ThrowIfFailed(LoadFromTGAFile(m_sourcePath.c_str(), m_metadata.get(), *m_scratchImage));
    } else {
        ThrowIfFailed(LoadFromWICFile(m_sourcePath.c_str(), WIC_FLAGS_FORCE_RGB, m_metadata.get(), *m_scratchImage));
    }

    // Force the texture format to be sRGB to convert to linear when sampling the texture in a shader.
    if (m_sRGB)
    {
        m_metadata->format = MakeSRGB(m_metadata->format);
    }
}


std::shared_ptr<DTexture> DTexture::LoadFromFile(const std::wstring& filePath, bool sRGB)
{
    return std::make_shared<DTexture>(filePath, sRGB);
}
