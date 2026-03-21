#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <memory>
#include <string>

#include "Core/DObject.h"
#include "Serialization/ISerializationCallbackReceiver.h"
#include "Serialization/TBulkData.h"

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

    /// Loads texture data from the given file path.
    DELTAENGINE_API void Initialize(const std::wstring& filePath, bool sRGB = false);
    /// Returns the texture width in pixels.
    DFUNCTION()
    DELTAENGINE_API UINT GetWidth() const;
    /// Returns the texture height in pixels.
    DFUNCTION()
    DELTAENGINE_API UINT GetHeight() const;
    
    /// Returns the loaded texture format.
    DELTAENGINE_API DXGI_FORMAT GetFormat() const;

    /// Returns the loaded DirectXTex metadata.
    inline std::shared_ptr<DirectX::TexMetadata> GetMetadata() const { return m_metadata; }
    /// Returns the loaded DirectXTex scratch image.
    inline std::shared_ptr<DirectX::ScratchImage> GetScratchImage() const { return m_scratchImage; }

    /// Returns the original texture source path.
    DFUNCTION()
    DELTAENGINE_API std::wstring GetSourcePath() const;

    /// Packs texture data into bulk storage before serialization.
    void OnBeforeSerialize() override;
    /// Restores texture data from bulk storage after deserialization.
    void OnAfterDeserialize() override;

private:
    void LoadTexture();
    std::shared_ptr<DirectX::TexMetadata> m_metadata;
    std::shared_ptr<DirectX::ScratchImage> m_scratchImage;

    DPROPERTY()
    TBulkData m_bulkData;

    DPROPERTY()
    std::wstring m_sourcePath;

    DPROPERTY()
    bool m_sRGB;

public:
    /// Creates and loads a texture from disk.
    static DTexture* LoadFromFile(const std::wstring& filePath, bool sRGB = false);
};

DELTA_ENGINE_NS_END
