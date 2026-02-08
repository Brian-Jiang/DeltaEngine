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
    void Resize(uint32_t width, uint32_t height, uint32_t depthOrArraySize = 1);

    D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(uint32_t mip) const;

    bool CheckSRVSupport() const { return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE); }
    bool CheckRTVSupport() const { return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_RENDER_TARGET); }
    bool CheckUAVSupport() const {
        return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) &&
               CheckFormatSupport(D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) &&
               CheckFormatSupport(D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE);
    }
    bool CheckDSVSupport() const { return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL); }

    bool HasAlpha() const;
    size_t BitsPerPixel() const;

    static bool IsUAVCompatibleFormat(DXGI_FORMAT format);
    static bool IsSRGBFormat(DXGI_FORMAT format);
    static bool IsBGRFormat(DXGI_FORMAT format);
    static bool IsDepthFormat(DXGI_FORMAT format);
    static DXGI_FORMAT GetTypelessFormat(DXGI_FORMAT format);
    static DXGI_FORMAT GetSRGBFormat(DXGI_FORMAT format);
    static DXGI_FORMAT GetUAVCompatableFormat(DXGI_FORMAT format);

public:
    DirectX12Texture(Device& device, const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue = nullptr);
    DirectX12Texture(Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource, const D3D12_CLEAR_VALUE* clearValue = nullptr);
    virtual ~DirectX12Texture();

    void CreateViews();

private:
    DescriptorAllocation m_RenderTargetView;
    DescriptorAllocation m_DepthStencilView;
    DescriptorAllocation m_ShaderResourceView;
    DescriptorAllocation m_UnorderedAccessView;
};

DELTA_ENGINE_NS_END
