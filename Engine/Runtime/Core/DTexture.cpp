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
    m_metadata = std::make_shared<TexMetadata>();
    m_scratchImage = std::make_shared<ScratchImage>();
}

//DTexture::DTexture(const std::wstring& filePath, bool sRGB)
//    : m_sourcePath(filePath)
//    , m_sRGB(sRGB)
//{
//    m_metadata = std::make_shared<TexMetadata>();
//    m_scratchImage = std::make_shared<ScratchImage>();
//    LoadTexture();
//}

DeltaEngine::DTexture::~DTexture()
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

UINT DeltaEngine::DTexture::GetWidth() const { return m_metadata->width; }

UINT DeltaEngine::DTexture::GetHeight() const { return m_metadata->height; }

DXGI_FORMAT DeltaEngine::DTexture::GetFormat() const { return m_metadata->format; }

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
        m_scratchImage->OverrideFormat(m_metadata->format);
    }
}


std::wstring DeltaEngine::DTexture::GetSourcePath() const { return m_sourcePath; }


DTexture* DTexture::LoadFromFile(const std::wstring& filePath, bool sRGB)
{
    DTexture* texture = CreateDObject<DTexture>();
    texture->Initialize(filePath, sRGB);
    return texture;
}

TBulkData SerializeTexture(
    const std::shared_ptr<DirectX::TexMetadata>& metadata,
    const std::shared_ptr<DirectX::ScratchImage>& scratchImage)
{
    assert(metadata && scratchImage);

    // Compress to BC3 if the image is currently uncompressed RGBA/BGRA.
    // BC7 gives better quality but is very slow to compress on CPU.
    const DXGI_FORMAT srcFmt = metadata->format;
    const bool needsCompress = !DirectX::IsCompressed(srcFmt);

    DirectX::ScratchImage compressedStorage;
    const DirectX::ScratchImage* imageToSerialize = scratchImage.get();
    DirectX::TexMetadata metaToSerialize = *metadata;

    if (needsCompress)
    {
        // Generate mips first if the source only has 1 level.
        DirectX::ScratchImage mippedStorage;
        const DirectX::ScratchImage* mippedImage = scratchImage.get();

        //if (metadata->mipLevels == 1)
        //{
        //    HRESULT hr = DirectX::GenerateMipMaps(
        //        scratchImage->GetImages(),
        //        scratchImage->GetImageCount(),
        //        *metadata,
        //        DirectX::TEX_FILTER_DEFAULT,
        //        0, // full mip chain
        //        mippedStorage);

        //    if (SUCCEEDED(hr))
        //        mippedImage = &mippedStorage;
        //}

        HRESULT hr = DirectX::Compress(
            mippedImage->GetImages(),
            mippedImage->GetImageCount(),
            mippedImage->GetMetadata(),
            //DXGI_FORMAT_BC7_UNORM, // swap to BC7_UNORM for higher quality
            DXGI_FORMAT_BC3_UNORM_SRGB, // swap to BC7_UNORM for higher quality
            DirectX::TEX_COMPRESS_PARALLEL,
            DirectX::TEX_THRESHOLD_DEFAULT,
            compressedStorage);

        if (SUCCEEDED(hr))
        {
            imageToSerialize = &compressedStorage;
            metaToSerialize = compressedStorage.GetMetadata();
        }
        else
        {
            printf("[DTexture] Compress failed (0x%08X), serializing raw", hr);
        }
    }

    // ── same layout as before, now with compressed data ───────────────────
    const uint64_t pixelSize = static_cast<uint64_t>(imageToSerialize->GetPixelsSize());
    const uint64_t totalSize = sizeof(DirectX::TexMetadata)
        + sizeof(uint64_t)
        + pixelSize;

    auto* buf = new uint8_t[totalSize];
    uint8_t* cursor = buf;

    std::memcpy(cursor, &metaToSerialize, sizeof(DirectX::TexMetadata));
    cursor += sizeof(DirectX::TexMetadata);

    std::memcpy(cursor, &pixelSize, sizeof(uint64_t));
    cursor += sizeof(uint64_t);

    if (pixelSize > 0)
        std::memcpy(cursor, imageToSerialize->GetPixels(), pixelSize);

    TBulkData bulk;
    bulk.Set(buf, totalSize);
    delete[] buf;
    return bulk;
}

bool DeserializeTexture(
    const TBulkData& bulk,
    std::shared_ptr<DirectX::TexMetadata>& outMetadata,
    std::shared_ptr<DirectX::ScratchImage>& outScratchImage)
{
    if (!bulk.IsValid())
        return false;

    const uint8_t* cursor = bulk.m_data;

    // ── 1. Metadata ────────────────────────────────────────────────────────
    auto meta = std::make_shared<DirectX::TexMetadata>();
    std::memcpy(meta.get(), cursor, sizeof(DirectX::TexMetadata));
    cursor += sizeof(DirectX::TexMetadata);

    // ── 2. Pixel blob size ─────────────────────────────────────────────────
    uint64_t pixelSize = 0;
    std::memcpy(&pixelSize, cursor, sizeof(uint64_t));
    cursor += sizeof(uint64_t);

    // ── 3. ScratchImage ────────────────────────────────────────────────────
    // Initialize allocates the correct mip/array layout from metadata alone.
    // The pixel pointer and slice offsets are wired up internally.
    auto scratch = std::make_shared<DirectX::ScratchImage>();

    HRESULT hr = scratch->Initialize(*meta);
    if (FAILED(hr)) {
        printf("[DTexture] DeserializeTexture: ScratchImage::Initialize failed (HRESULT 0x%08X)", hr);
        return false;
    }

    if (scratch->GetPixelsSize() != static_cast<size_t>(pixelSize)) {
        printf("[DTexture] DeserializeTexture: pixel size mismatch — expected %llu, got %zu",
            pixelSize, scratch->GetPixelsSize());
        return false;
    }

    std::memcpy(scratch->GetPixels(), cursor, pixelSize);

    outMetadata = std::move(meta);
    outScratchImage = std::move(scratch);
    return true;
}

void DTexture::OnBeforeSerialize()
{
    m_bulkData = SerializeTexture(m_metadata, m_scratchImage);
}

void DTexture::OnAfterDeserialize()
{
    std::shared_ptr<TexMetadata> meta;
    std::shared_ptr<ScratchImage> scratch;
    if (DeserializeTexture(m_bulkData, meta, scratch))
    {
        m_metadata = std::move(meta);
        m_scratchImage = std::move(scratch);
    }
    else
    {
        m_metadata = std::make_shared<TexMetadata>();
        m_scratchImage = std::make_shared<ScratchImage>();
    }

    if (m_sRGB) {
        m_metadata->format = MakeSRGB(m_metadata->format);
        m_scratchImage->OverrideFormat(m_metadata->format);
    }
}
