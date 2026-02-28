#pragma once

#include "EngineIncludes.h"

#include <vector>
#include <string>
#include <memory>
#include <d3d12.h>

#include "Core/DObject.h"

#include "DTexture.generated.h"

namespace DirectX
{
    struct TexMetadata;
    class ScratchImage;
}


DELTA_ENGINE_NS_BEGIN

DCLASS()
class DTexture : public DObject, public std::enable_shared_from_this<DTexture>
{
    DGENERATED_BODY(DTexture)

public:
    DTexture();
    DTexture(const std::wstring& filePath, bool sRGB);
    ~DTexture();

    DFUNCTION()
    UINT GetWidth() const;
    DFUNCTION()
    UINT GetHeight() const;
    DFUNCTION()
    DXGI_FORMAT GetFormat() const;

    DFUNCTION()
    inline std::shared_ptr<DirectX::TexMetadata> GetMetadata() const { return m_metadata; }
    DFUNCTION()
    inline std::shared_ptr<DirectX::ScratchImage> GetScratchImage() const { return m_scratchImage; }
    DFUNCTION()
    inline std::wstring GetSourcePath() const { return m_sourcePath; }

private:
    void LoadTexture();
    

private:
    DPROPERTY()
    std::shared_ptr<DirectX::TexMetadata> m_metadata;
    DPROPERTY()
    std::shared_ptr<DirectX::ScratchImage> m_scratchImage;
    DPROPERTY()
    std::wstring m_sourcePath;
    DPROPERTY()
    bool m_sRGB;


public:
    DFUNCTION()
    static std::shared_ptr<DTexture> LoadFromFile(const std::wstring& filePath, bool sRGB = false);
};

DELTA_ENGINE_NS_END
