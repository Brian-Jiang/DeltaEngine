#pragma once

#include "EngineIncludes.h"

#include <vector>
#include <string>
#include <memory>
#include <d3d12.h>

namespace DirectX
{
    struct TexMetadata;
    class ScratchImage;
}


DELTA_ENGINE_NS_BEGIN

class DTexture
{
public:
    DTexture();
    DTexture(const std::wstring& filePath, bool sRGB);
    ~DTexture();

    UINT GetWidth() const;
    UINT GetHeight() const;
    DXGI_FORMAT GetFormat() const;

private:
    void LoadTexture();
    

private:
    std::shared_ptr<DirectX::TexMetadata> m_metadata;
    std::shared_ptr<DirectX::ScratchImage> m_scratchImage;
    std::wstring m_sourcePath;
    bool m_sRGB;


public:
    static std::shared_ptr<DTexture> LoadFromFile(const std::string& filePath, bool sRGB = false);
};

DELTA_ENGINE_NS_END
