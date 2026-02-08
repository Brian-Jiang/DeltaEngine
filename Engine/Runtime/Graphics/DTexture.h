#pragma once

#include "EngineIncludes.h"

#include <vector>
#include <string>
#include <memory>
#include <d3d12.h>

DELTA_ENGINE_NS_BEGIN

class DirectX12Texture;
class Device;
class CommandList;

/// Engine-level texture. Load from file, then upload to GPU via Device::CreateTextureFromFile.
/// Renderers only interact with DTexture; descriptor heap and SRV are managed internally.
class DTexture
{
public:
    unsigned int GetWidth() const { return m_width; }
    unsigned int GetHeight() const { return m_height; }
    const std::vector<unsigned char>& GetData() const { return m_data; }
    DXGI_FORMAT GetFormat() const { return m_format; }
    const std::string& GetName() const { return m_name; }

    static std::shared_ptr<DTexture> LoadFromFile(const std::string& filePath, bool isFullPath = false);

    /// For binding: SRV descriptor (valid after CreateTextureFromFile). Returns null handle if not yet on GPU.
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUHandle() const;
    /// For resource state transitions. Returns nullptr if not yet on GPU.
    ID3D12Resource* GetD3D12Resource() const;

    /// Called by Device when GPU texture is created. Not for use by renderers.
    void SetGPUTexture(std::shared_ptr<DirectX12Texture> dx12Texture);


    DTexture(const std::string& filePath, bool isFullPath);

private:
    unsigned int m_width = 0;
    unsigned int m_height = 0;
    std::vector<unsigned char> m_data;
    int m_pixelSize = 0;
    DXGI_FORMAT m_format = DXGI_FORMAT_R8G8B8A8_UNORM;
    std::string m_name;

    std::shared_ptr<DirectX12Texture> m_dx12Texture;
};

DELTA_ENGINE_NS_END
