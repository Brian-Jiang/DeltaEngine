#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/DirectX/DescriptorAllocation.h"

#include <d3d12.h>
#include <memory>
#include <string>

DELTA_ENGINE_NS_BEGIN

class Device;
class DirectX12Texture;

class ShadowCubeArray
{
public:
    DELTAENGINE_API void Initialize(Device& device, uint32_t faceSize, uint32_t cubeCount, const std::string& debugNameUtf8);
    DELTAENGINE_API void Shutdown();

    DELTAENGINE_API std::shared_ptr<DirectX12Texture> GetTexture() const { return m_texture; }
    DELTAENGINE_API D3D12_CPU_DESCRIPTOR_HANDLE GetDSVForFace(uint32_t cubeIndex, uint32_t face) const;
    DELTAENGINE_API D3D12_CPU_DESCRIPTOR_HANDLE GetFullDSV() const;
    DELTAENGINE_API D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const;
    DELTAENGINE_API uint32_t GetFaceSize() const { return m_faceSize; }
    DELTAENGINE_API uint32_t GetCubeCount() const { return m_cubeCount; }

private:
    std::shared_ptr<DirectX12Texture> m_texture;
    DescriptorAllocation m_faceDSVs;
    uint32_t m_faceSize = 0;
    uint32_t m_cubeCount = 0;
};

DELTA_ENGINE_NS_END
