#pragma once

#include "EngineIncludes.h"

#include "Runtime/Core/DObject.h"
#include "Runtime/Serialization/ISerializationCallbackReceiver.h"
#include "Runtime/Serialization/TBulkData.h"

#include <d3d12.h>

#include <filesystem>
#include <memory>
#include <string>

#include "DTexture.generated.h"

namespace DirectX
{
    struct TexMetadata;
    class ScratchImage;
}


DELTA_ENGINE_NS_BEGIN

DCLASS()
class DTexture : public DObject, public ISerializationCallbackReceiver
{
    DGENERATED_BODY(DTexture)

public:
    DELTAENGINE_API DTexture();
    DELTAENGINE_API ~DTexture();

    DELTAENGINE_API void Initialize(const std::filesystem::path& filePath, bool sRGB = false);
    DFUNCTION()
    DELTAENGINE_API UINT GetWidth() const;
    DFUNCTION()
    DELTAENGINE_API UINT GetHeight() const;

    DELTAENGINE_API DXGI_FORMAT GetFormat() const;
    DELTAENGINE_API bool IsCubemap() const;

    inline std::shared_ptr<DirectX::TexMetadata> GetMetadata() const { return m_metadata; }
    inline std::shared_ptr<DirectX::ScratchImage> GetScratchImage() const { return m_scratchImage; }

    DFUNCTION()
    DELTAENGINE_API std::filesystem::path GetSourcePath() const;

    void OnBeforeSerialize() override;
    void OnAfterDeserialize() override;

private:
    void LoadTexture();
    std::shared_ptr<DirectX::TexMetadata> m_metadata;
    std::shared_ptr<DirectX::ScratchImage> m_scratchImage;

    DPROPERTY()
    TBulkData m_bulkData;

    DPROPERTY()
    std::filesystem::path m_sourcePath;

    DPROPERTY()
    bool m_sRGB;

public:
    DELTAENGINE_API static DTexture* LoadFromFile(const std::filesystem::path& filePath, bool sRGB = false);
};

DELTA_ENGINE_NS_END
