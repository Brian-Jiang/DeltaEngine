#include "Core/DTexture.h"

#include "Runtime/Graphics/DXUtils.h"

#include <DirectXTex.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <stdexcept>

namespace
{
DeltaEngine::TBulkData SerializeTexture(
    const std::shared_ptr<DirectX::TexMetadata>& metadata,
    const std::shared_ptr<DirectX::ScratchImage>& scratchImage)
{
    assert(metadata && scratchImage);

    const bool needsCompression = !DirectX::IsCompressed(metadata->format);

    DirectX::ScratchImage compressedStorage;
    const DirectX::ScratchImage* imageToSerialize = scratchImage.get();
    DirectX::TexMetadata metadataToSerialize = *metadata;

    if (needsCompression)
    {
        const HRESULT hr = DirectX::Compress(
            scratchImage->GetImages(),
            scratchImage->GetImageCount(),
            *metadata,
            DXGI_FORMAT_BC3_UNORM_SRGB,
            DirectX::TEX_COMPRESS_PARALLEL,
            DirectX::TEX_THRESHOLD_DEFAULT,
            compressedStorage);

        if (SUCCEEDED(hr))
        {
            imageToSerialize = &compressedStorage;
            metadataToSerialize = compressedStorage.GetMetadata();
        }
        else
        {
            std::printf("[DTexture] Compress failed (0x%08X), serializing raw", hr);
        }
    }

    const uint64_t pixelSize = static_cast<uint64_t>(imageToSerialize->GetPixelsSize());
    const uint64_t totalSize = sizeof(DirectX::TexMetadata) + sizeof(uint64_t) + pixelSize;

    auto* buffer = new uint8_t[totalSize];
    uint8_t* cursor = buffer;

    std::memcpy(cursor, &metadataToSerialize, sizeof(DirectX::TexMetadata));
    cursor += sizeof(DirectX::TexMetadata);

    std::memcpy(cursor, &pixelSize, sizeof(uint64_t));
    cursor += sizeof(uint64_t);

    if (pixelSize > 0)
        std::memcpy(cursor, imageToSerialize->GetPixels(), pixelSize);

    DeltaEngine::TBulkData bulk;
    bulk.Set(buffer, totalSize);
    delete[] buffer;
    return bulk;
}

bool DeserializeTexture(
    const DeltaEngine::TBulkData& bulk,
    std::shared_ptr<DirectX::TexMetadata>& outMetadata,
    std::shared_ptr<DirectX::ScratchImage>& outScratchImage)
{
    if (!bulk.IsValid())
        return false;

    const uint8_t* cursor = bulk.m_data;

    auto metadata = std::make_shared<DirectX::TexMetadata>();
    std::memcpy(metadata.get(), cursor, sizeof(DirectX::TexMetadata));
    cursor += sizeof(DirectX::TexMetadata);

    uint64_t pixelSize = 0;
    std::memcpy(&pixelSize, cursor, sizeof(uint64_t));
    cursor += sizeof(uint64_t);

    auto scratchImage = std::make_shared<DirectX::ScratchImage>();
    const HRESULT hr = scratchImage->Initialize(*metadata);
    if (FAILED(hr))
    {
        std::printf("[DTexture] DeserializeTexture: ScratchImage::Initialize failed (HRESULT 0x%08X)", hr);
        return false;
    }

    if (scratchImage->GetPixelsSize() != static_cast<size_t>(pixelSize))
    {
        std::printf(
            "[DTexture] DeserializeTexture: pixel size mismatch - expected %llu, got %zu",
            static_cast<unsigned long long>(pixelSize),
            scratchImage->GetPixelsSize());
        return false;
    }

    std::memcpy(scratchImage->GetPixels(), cursor, pixelSize);

    outMetadata = std::move(metadata);
    outScratchImage = std::move(scratchImage);
    return true;
}
}

using namespace DeltaEngine;
using namespace DirectX;

DTexture::DTexture()
    : m_sourcePath(L"")
    , m_sRGB(false)
{
    m_metadata = std::make_shared<TexMetadata>();
    m_scratchImage = std::make_shared<ScratchImage>();
}

DTexture::~DTexture()
{
    m_scratchImage.reset();
}

void DTexture::Initialize(const std::wstring& filePath, bool sRGB)
{
    m_sourcePath = filePath;
    m_sRGB = sRGB;
    m_metadata = std::make_shared<TexMetadata>();
    m_scratchImage = std::make_shared<ScratchImage>();
    LoadTexture();
}

UINT DTexture::GetWidth() const { return m_metadata->width; }

UINT DTexture::GetHeight() const { return m_metadata->height; }

DXGI_FORMAT DTexture::GetFormat() const { return m_metadata->format; }

void DTexture::LoadTexture()
{
    const std::filesystem::path filePath(m_sourcePath);
    if (!std::filesystem::exists(filePath))
        throw std::runtime_error("Texture file not found.");

    if (filePath.extension() == ".dds")
    {
        ThrowIfFailed(LoadFromDDSFile(m_sourcePath.c_str(), DDS_FLAGS_FORCE_RGB, m_metadata.get(), *m_scratchImage));
    }
    else if (filePath.extension() == ".hdr")
    {
        ThrowIfFailed(LoadFromHDRFile(m_sourcePath.c_str(), m_metadata.get(), *m_scratchImage));
    }
    else if (filePath.extension() == ".tga")
    {
        ThrowIfFailed(LoadFromTGAFile(m_sourcePath.c_str(), m_metadata.get(), *m_scratchImage));
    }
    else
    {
        ThrowIfFailed(LoadFromWICFile(m_sourcePath.c_str(), WIC_FLAGS_FORCE_RGB, m_metadata.get(), *m_scratchImage));
    }

    if (m_sRGB)
    {
        m_metadata->format = MakeSRGB(m_metadata->format);
        m_scratchImage->OverrideFormat(m_metadata->format);
    }
}

std::wstring DTexture::GetSourcePath() const
{
    return m_sourcePath;
}

bool DTexture::IsCubemap() const
{
    return m_metadata && m_metadata->IsCubemap();
}

DTexture* DTexture::LoadFromFile(const std::wstring& filePath, bool sRGB)
{
    DTexture* texture = CreateDObject<DTexture>();
    texture->Initialize(filePath, sRGB);
    return texture;
}

void DTexture::OnBeforeSerialize()
{
    m_bulkData = SerializeTexture(m_metadata, m_scratchImage);
}

void DTexture::OnAfterDeserialize()
{
    std::shared_ptr<TexMetadata> metadata;
    std::shared_ptr<ScratchImage> scratchImage;
    if (DeserializeTexture(m_bulkData, metadata, scratchImage))
    {
        m_metadata = std::move(metadata);
        m_scratchImage = std::move(scratchImage);
    }
    else
    {
        m_metadata = std::make_shared<TexMetadata>();
        m_scratchImage = std::make_shared<ScratchImage>();
    }

    if (m_sRGB)
    {
        m_metadata->format = MakeSRGB(m_metadata->format);
        m_scratchImage->OverrideFormat(m_metadata->format);
    }
}
