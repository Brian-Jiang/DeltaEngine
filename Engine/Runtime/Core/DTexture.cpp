#include "Runtime/Core/DTexture.h"

#include "Runtime/Graphics/DXUtils.h"

#include <DirectXTex.h>

#include <cstring>
#include <filesystem>
#include <stdexcept>

namespace
{
std::string PathLog(const std::filesystem::path& p)
{
    const std::u8string u = p.u8string();
    return { reinterpret_cast<const char*>(u.data()), u.size() };
}

DeltaEngine::TBulkData SerializeTexture(
    const std::shared_ptr<DirectX::TexMetadata>& metadata,
    const std::shared_ptr<DirectX::ScratchImage>& scratchImage)
{
    if (!metadata || !scratchImage)
    {
        DLOG(LogAsset, ELogLevel::Warning,
            "SerializeTexture: missing metadata ({}) or scratchImage ({}) — writing empty bulk",
            static_cast<bool>(metadata),
            static_cast<bool>(scratchImage));
        return {};
    }

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
            DLOG(LogAsset, ELogLevel::Warning,
                "SerializeTexture: Compress failed HRESULT={:#010x} — serializing uncompressed pixels",
                static_cast<unsigned int>(hr));
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

    constexpr uint64_t kHeader = sizeof(DirectX::TexMetadata) + sizeof(uint64_t);
    if (bulk.m_size < kHeader)
    {
        DLOG(LogAsset, ELogLevel::Warning,
            "DeserializeTexture: bulk too small for header have {} need {}",
            bulk.m_size, kHeader);
        return false;
    }

    size_t off = 0;
    const uint8_t* base = bulk.m_data;

    auto metadata = std::make_shared<DirectX::TexMetadata>();
    std::memcpy(metadata.get(), base + off, sizeof(DirectX::TexMetadata));
    off += sizeof(DirectX::TexMetadata);

    uint64_t pixelSize = 0;
    std::memcpy(&pixelSize, base + off, sizeof(uint64_t));
    off += sizeof(uint64_t);

    if (bulk.m_size < off + pixelSize)
    {
        DLOG(LogAsset, ELogLevel::Warning,
            "DeserializeTexture: truncated pixel payload off={} pixelBytes={} bulkSize={}",
            static_cast<unsigned long long>(off),
            static_cast<unsigned long long>(pixelSize),
            static_cast<unsigned long long>(bulk.m_size));
        return false;
    }

    auto scratchImage = std::make_shared<DirectX::ScratchImage>();
    const HRESULT hr = scratchImage->Initialize(*metadata);
    if (FAILED(hr))
    {
        DLOG(LogAsset, ELogLevel::Warning,
            "DeserializeTexture: ScratchImage::Initialize failed HRESULT={:#010x}",
            static_cast<unsigned int>(hr));
        return false;
    }

    if (scratchImage->GetPixelsSize() != static_cast<size_t>(pixelSize))
    {
        DLOG(LogAsset, ELogLevel::Warning,
            "DeserializeTexture: pixel size mismatch expected {} got {}",
            static_cast<unsigned long long>(pixelSize),
            static_cast<unsigned long long>(scratchImage->GetPixelsSize()));
        return false;
    }

    if (pixelSize > 0)
        std::memcpy(scratchImage->GetPixels(), base + off, static_cast<size_t>(pixelSize));

    outMetadata = std::move(metadata);
    outScratchImage = std::move(scratchImage);
    return true;
}
}

using namespace DeltaEngine;
using namespace DirectX;

DTexture::DTexture()
    : m_sRGB(false)
{
    m_metadata = std::make_shared<TexMetadata>();
    m_scratchImage = std::make_shared<ScratchImage>();
}

DTexture::~DTexture()
{
    m_scratchImage.reset();
}

void DTexture::Initialize(const std::filesystem::path& filePath, bool sRGB)
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
    if (!std::filesystem::exists(m_sourcePath))
    {
        DLOG(LogAsset, ELogLevel::Error,
            "LoadTexture: file not found (resolved generic path '{}')",
            PathLog(m_sourcePath));
        throw std::runtime_error("Texture file not found.");
    }

    if (m_sourcePath.extension() == ".dds")
    {
        ThrowIfFailed(LoadFromDDSFile(m_sourcePath.c_str(), DDS_FLAGS_FORCE_RGB, m_metadata.get(), *m_scratchImage));
    }
    else if (m_sourcePath.extension() == ".hdr")
    {
        ThrowIfFailed(LoadFromHDRFile(m_sourcePath.c_str(), m_metadata.get(), *m_scratchImage));
    }
    else if (m_sourcePath.extension() == ".tga")
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

std::filesystem::path DTexture::GetSourcePath() const
{
    return m_sourcePath;
}

bool DTexture::IsCubemap() const
{
    return m_metadata && m_metadata->IsCubemap();
}

DTexture* DTexture::LoadFromFile(const std::filesystem::path& filePath, bool sRGB)
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
    if (!DeserializeTexture(m_bulkData, metadata, scratchImage))
    {
        DLOG(LogAsset, ELogLevel::Warning,
            "DTexture::OnAfterDeserialize: bulk restore failed bulkBytes={} source='{}' — resetting to empty image",
            m_bulkData.m_size,
            PathLog(m_sourcePath));
        m_metadata = std::make_shared<TexMetadata>();
        m_scratchImage = std::make_shared<ScratchImage>();
        return;
    }

    m_metadata = std::move(metadata);
    m_scratchImage = std::move(scratchImage);

    if (m_sRGB)
    {
        m_metadata->format = MakeSRGB(m_metadata->format);
        m_scratchImage->OverrideFormat(m_metadata->format);
    }
}
