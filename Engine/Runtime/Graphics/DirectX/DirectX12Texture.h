#pragma once

/**
 *  @file DirectX12Texture.h
 *  DX12-specific texture implementation (Resource + SRV/RTV/DSV/UAV).
 */

#include "EngineIncludes.h"

#include <d3dx12.h>
#include <mutex>
#include <unordered_map>

#include "Runtime/Graphics/DirectX/Resource.h"
#include "Runtime/Graphics/DirectX/DescriptorAllocation.h"

DELTA_ENGINE_NS_BEGIN

class Device;

class DirectX12Texture : public Resource
{
public:
    DELTAENGINE_API void Resize(uint32_t width, uint32_t height, uint32_t depthOrArraySize = 1);

    DELTAENGINE_API D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const;
    DELTAENGINE_API D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;
    DELTAENGINE_API D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView() const;
    DELTAENGINE_API D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(uint32_t mip) const;

    DELTAENGINE_API bool CheckSRVSupport() const { return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE); }
    DELTAENGINE_API bool CheckRTVSupport() const { return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_RENDER_TARGET); }
    DELTAENGINE_API bool CheckUAVSupport() const {
        return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) &&
               CheckFormatSupport(D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) &&
               CheckFormatSupport(D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE);
    }
    DELTAENGINE_API bool CheckDSVSupport() const { return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL); }

    DELTAENGINE_API bool HasAlpha() const;
    DELTAENGINE_API size_t BitsPerPixel() const;

    DELTAENGINE_API static bool IsUAVCompatibleFormat(DXGI_FORMAT format);
    DELTAENGINE_API static bool IsSRGBFormat(DXGI_FORMAT format);
    DELTAENGINE_API static bool IsBGRFormat(DXGI_FORMAT format);
    DELTAENGINE_API static bool IsDepthFormat(DXGI_FORMAT format);
    DELTAENGINE_API static DXGI_FORMAT GetTypelessFormat(DXGI_FORMAT format);
    DELTAENGINE_API static DXGI_FORMAT GetSRGBFormat(DXGI_FORMAT format);
    DELTAENGINE_API static DXGI_FORMAT GetUAVCompatableFormat(DXGI_FORMAT format);

public:
    DELTAENGINE_API DirectX12Texture(Device& device, const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue = nullptr);
    DELTAENGINE_API DirectX12Texture(Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource, const D3D12_CLEAR_VALUE* clearValue = nullptr);
    DELTAENGINE_API virtual ~DirectX12Texture();

    DELTAENGINE_API void CreateViews();

private:
    DescriptorAllocation m_RenderTargetView;
    DescriptorAllocation m_DepthStencilView;
    DescriptorAllocation m_ShaderResourceView;
    DescriptorAllocation m_UnorderedAccessView;
};

DELTA_ENGINE_NS_END
