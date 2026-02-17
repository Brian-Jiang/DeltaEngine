#pragma once

#include "EngineIncludes.h"

#include <vector>
#include <string>
#include <memory>
#include <d3d12.h>

#include "Core/DObject.h"

namespace DirectX
{
    struct TexMetadata;
    class ScratchImage;
}


DELTA_ENGINE_NS_BEGIN

class DTexture : public DObject, public std::enable_shared_from_this<DTexture>
{
public:
    DTexture();
    DTexture(const std::wstring& filePath, bool sRGB);
    ~DTexture();

    UINT GetWidth() const;
    UINT GetHeight() const;
    DXGI_FORMAT GetFormat() const;

    inline std::shared_ptr<DirectX::TexMetadata> GetMetadata() const { return m_metadata; }
    inline std::shared_ptr<DirectX::ScratchImage> GetScratchImage() const { return m_scratchImage; }
    inline std::wstring GetSourcePath() const { return m_sourcePath; }

private:
    void LoadTexture();
    

private:
    std::shared_ptr<DirectX::TexMetadata> m_metadata;
    std::shared_ptr<DirectX::ScratchImage> m_scratchImage;
    std::wstring m_sourcePath;
    bool m_sRGB;


public:
    static std::shared_ptr<DTexture> LoadFromFile(const std::wstring& filePath, bool sRGB = false);
};

DELTA_ENGINE_NS_END
