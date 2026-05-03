#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <memory>
#include <string>

DELTA_ENGINE_NS_BEGIN

class Device;
class DirectX12Texture;

class ShadowAtlas
{
public:
    DELTAENGINE_API void Initialize(Device& device, uint32_t widthHeight, const std::string& debugNameUtf8);
    DELTAENGINE_API void Shutdown();

    DELTAENGINE_API std::shared_ptr<DirectX12Texture> GetTexture() const { return m_texture; }
    DELTAENGINE_API D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const;
    DELTAENGINE_API D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const;
    DELTAENGINE_API uint32_t GetSize() const { return m_size; }

private:
    std::shared_ptr<DirectX12Texture> m_texture;
    uint32_t m_size = 0;
};

DELTA_ENGINE_NS_END
