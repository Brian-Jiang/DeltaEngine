#pragma once

#include "EngineIncludes.h"

#include <vector>
#include <string>
#include <memory>
#include <d3d12.h>

#include "Core/DObject.h"
#include "Serialization/TBulkData.h"
#include "Serialization/ISerializationCallbackReceiver.h"

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
    DTexture();
    //DTexture(const std::wstring& filePath, bool sRGB);
    ~DTexture();

    void Initialize(const std::wstring& filePath, bool sRGB = false);

    DFUNCTION()
    UINT GetWidth() const;
    DFUNCTION()
    UINT GetHeight() const;
    
    DXGI_FORMAT GetFormat() const;

    
    inline std::shared_ptr<DirectX::TexMetadata> GetMetadata() const { return m_metadata; }
    inline std::shared_ptr<DirectX::ScratchImage> GetScratchImage() const { return m_scratchImage; }

    DFUNCTION()
    DELTAENGINE_API std::wstring GetSourcePath() const;

    void OnBeforeSerialize() override;
    void OnAfterDeserialize() override;

private:
    void LoadTexture();
    

private:
    std::shared_ptr<DirectX::TexMetadata> m_metadata;
    std::shared_ptr<DirectX::ScratchImage> m_scratchImage;

    DPROPERTY()
    TBulkData m_bulkData;

    DPROPERTY()
    std::wstring m_sourcePath;

    DPROPERTY()
    bool m_sRGB;


public:
    static DTexture* LoadFromFile(const std::wstring& filePath, bool sRGB = false);
};

DELTA_ENGINE_NS_END
