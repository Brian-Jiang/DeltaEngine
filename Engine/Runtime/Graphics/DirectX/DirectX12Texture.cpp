#include "Runtime/Graphics/DirectX/DirectX12Texture.h"

#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/ResourceStateTracker.h"
#include "Runtime/Graphics/DXUtils.h"
#include "DirectXTex.h"

using namespace DeltaEngine;
using namespace Microsoft::WRL;

DirectX12Texture::DirectX12Texture(Device& device, const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue)
    : Resource(device, resourceDesc, clearValue)
{
    CreateViews();
}

DirectX12Texture::DirectX12Texture(Device& device, ComPtr<ID3D12Resource> resource, const D3D12_CLEAR_VALUE* clearValue)
    : Resource(device, resource, clearValue)
{
    CreateViews();
}

DirectX12Texture::~DirectX12Texture() {}

void DirectX12Texture::Resize(uint32_t width, uint32_t height, uint32_t depthOrArraySize)
{
    if (m_d3d12Resource)
    {
        CD3DX12_RESOURCE_DESC resDesc(m_d3d12Resource->GetDesc());
        resDesc.Width = std::max(width, 1u);
        resDesc.Height = std::max(height, 1u);
        resDesc.DepthOrArraySize = depthOrArraySize;
        resDesc.MipLevels = resDesc.SampleDesc.Count > 1 ? 1 : 0;

        auto d3d12Device = m_Device.GetD3D12Device();
        auto heapPorp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        ThrowIfFailed(d3d12Device->CreateCommittedResource(
            &heapPorp, D3D12_HEAP_FLAG_NONE, &resDesc,
            D3D12_RESOURCE_STATE_COMMON, m_d3d12ClearValue.get(), IID_PPV_ARGS(&m_d3d12Resource)));

        m_d3d12Resource->SetName(m_ResourceName.c_str());
        ResourceStateTracker::AddGlobalResourceState(m_d3d12Resource.Get(), D3D12_RESOURCE_STATE_COMMON);
        CreateViews();
    }
}

static D3D12_UNORDERED_ACCESS_VIEW_DESC GetUAVDesc(const D3D12_RESOURCE_DESC& resDesc, UINT mipSlice, UINT arraySlice = 0, UINT planeSlice = 0)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = resDesc.Format;
    switch (resDesc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        if (resDesc.DepthOrArraySize > 1) {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            uavDesc.Texture1DArray.ArraySize = resDesc.DepthOrArraySize - arraySlice;
            uavDesc.Texture1DArray.FirstArraySlice = arraySlice;
            uavDesc.Texture1DArray.MipSlice = mipSlice;
        } else {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
            uavDesc.Texture1D.MipSlice = mipSlice;
        }
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        if (resDesc.DepthOrArraySize > 1) {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            uavDesc.Texture2DArray.ArraySize = resDesc.DepthOrArraySize - arraySlice;
            uavDesc.Texture2DArray.FirstArraySlice = arraySlice;
            uavDesc.Texture2DArray.PlaneSlice = planeSlice;
            uavDesc.Texture2DArray.MipSlice = mipSlice;
        } else {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.PlaneSlice = planeSlice;
            uavDesc.Texture2D.MipSlice = mipSlice;
        }
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        uavDesc.Texture3D.WSize = resDesc.DepthOrArraySize - arraySlice;
        uavDesc.Texture3D.FirstWSlice = arraySlice;
        uavDesc.Texture3D.MipSlice = mipSlice;
        break;
    default:
        throw std::exception("Invalid resource dimension.");
    }
    return uavDesc;
}

void DirectX12Texture::CreateViews()
{
    if (m_d3d12Resource)
    {
        auto d3d12Device = m_Device.GetD3D12Device();
        CD3DX12_RESOURCE_DESC desc(m_d3d12Resource->GetDesc());

        if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0 && CheckRTVSupport()) {
            m_RenderTargetView = m_Device.AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            d3d12Device->CreateRenderTargetView(m_d3d12Resource.Get(), nullptr, m_RenderTargetView.GetDescriptorHandle());
        }
        if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0 && CheckDSVSupport()) {
            m_DepthStencilView = m_Device.AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            d3d12Device->CreateDepthStencilView(m_d3d12Resource.Get(), nullptr, m_DepthStencilView.GetDescriptorHandle());
        }
        if ((desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0 && CheckSRVSupport()) {
            m_ShaderResourceView = m_Device.AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            d3d12Device->CreateShaderResourceView(m_d3d12Resource.Get(), nullptr, m_ShaderResourceView.GetDescriptorHandle());
        }
        if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0 && CheckUAVSupport() && desc.DepthOrArraySize == 1) {
            m_UnorderedAccessView = m_Device.AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, desc.MipLevels);
            for (int i = 0; i < desc.MipLevels; ++i) {
                auto uavDesc = GetUAVDesc(desc, i);
                d3d12Device->CreateUnorderedAccessView(m_d3d12Resource.Get(), nullptr, &uavDesc, m_UnorderedAccessView.GetDescriptorHandle(i));
            }
        }
    }
}

void DirectX12Texture::CreateCubemapSRV()
{
    if (!m_d3d12Resource)
        return;

    auto d3d12Device = m_Device.GetD3D12Device();
    CD3DX12_RESOURCE_DESC desc(m_d3d12Resource->GetDesc());

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                          = desc.Format;
    srvDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MostDetailedMip     = 0;
    srvDesc.TextureCube.MipLevels           = desc.MipLevels;
    srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

    m_ShaderResourceView = m_Device.AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    d3d12Device->CreateShaderResourceView(
        m_d3d12Resource.Get(), &srvDesc,
        m_ShaderResourceView.GetDescriptorHandle());
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectX12Texture::GetRenderTargetView() const { return m_RenderTargetView.GetDescriptorHandle(); }
D3D12_CPU_DESCRIPTOR_HANDLE DirectX12Texture::GetDepthStencilView() const { return m_DepthStencilView.GetDescriptorHandle(); }
D3D12_CPU_DESCRIPTOR_HANDLE DirectX12Texture::GetShaderResourceView() const { return m_ShaderResourceView.GetDescriptorHandle(); }
D3D12_CPU_DESCRIPTOR_HANDLE DirectX12Texture::GetUnorderedAccessView(uint32_t mip) const { return m_UnorderedAccessView.GetDescriptorHandle(mip); }

bool DirectX12Texture::HasAlpha() const
{
    DXGI_FORMAT format = GetD3D12ResourceDesc().Format;
    bool hasAlpha = false;
    switch (format) {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS: case DXGI_FORMAT_R32G32B32A32_FLOAT: case DXGI_FORMAT_R32G32B32A32_UINT: case DXGI_FORMAT_R32G32B32A32_SINT:
    case DXGI_FORMAT_R16G16B16A16_TYPELESS: case DXGI_FORMAT_R16G16B16A16_FLOAT: case DXGI_FORMAT_R16G16B16A16_UNORM: case DXGI_FORMAT_R16G16B16A16_UINT: case DXGI_FORMAT_R16G16B16A16_SNORM: case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R10G10B10A2_TYPELESS: case DXGI_FORMAT_R10G10B10A2_UNORM: case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS: case DXGI_FORMAT_R8G8B8A8_UNORM: case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: case DXGI_FORMAT_R8G8B8A8_UINT: case DXGI_FORMAT_R8G8B8A8_SNORM: case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_BC1_TYPELESS: case DXGI_FORMAT_BC1_UNORM: case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_TYPELESS: case DXGI_FORMAT_BC2_UNORM: case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS: case DXGI_FORMAT_BC3_UNORM: case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_B5G5R5A1_UNORM: case DXGI_FORMAT_B8G8R8A8_UNORM: case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM: case DXGI_FORMAT_B8G8R8A8_TYPELESS: case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: case DXGI_FORMAT_B8G8R8X8_TYPELESS: case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_BC6H_TYPELESS: case DXGI_FORMAT_BC7_TYPELESS: case DXGI_FORMAT_BC7_UNORM: case DXGI_FORMAT_BC7_UNORM_SRGB:
    case DXGI_FORMAT_A8P8: case DXGI_FORMAT_B4G4R4A4_UNORM:
        hasAlpha = true; break;
    }
    return hasAlpha;
}

size_t DirectX12Texture::BitsPerPixel() const { return DirectX::BitsPerPixel(GetD3D12ResourceDesc().Format); }

bool DirectX12Texture::IsUAVCompatibleFormat(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_R32G32B32A32_FLOAT: case DXGI_FORMAT_R32G32B32A32_UINT: case DXGI_FORMAT_R32G32B32A32_SINT:
    case DXGI_FORMAT_R16G16B16A16_FLOAT: case DXGI_FORMAT_R16G16B16A16_UINT: case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R8G8B8A8_UNORM: case DXGI_FORMAT_R8G8B8A8_UINT: case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R32_FLOAT: case DXGI_FORMAT_R32_UINT: case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R16_FLOAT: case DXGI_FORMAT_R16_UINT: case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_R8_UNORM: case DXGI_FORMAT_R8_UINT: case DXGI_FORMAT_R8_SINT:
        return true;
    default: return false;
    }
}

bool DirectX12Texture::IsSRGBFormat(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: case DXGI_FORMAT_BC1_UNORM_SRGB: case DXGI_FORMAT_BC2_UNORM_SRGB: case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: case DXGI_FORMAT_BC7_UNORM_SRGB:
        return true;
    default: return false;
    }
}

bool DirectX12Texture::IsBGRFormat(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_B8G8R8A8_UNORM: case DXGI_FORMAT_B8G8R8X8_UNORM: case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: case DXGI_FORMAT_B8G8R8X8_TYPELESS: case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return true;
    default: return false;
    }
}

bool DirectX12Texture::IsDepthFormat(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: case DXGI_FORMAT_D32_FLOAT: case DXGI_FORMAT_D24_UNORM_S8_UINT: case DXGI_FORMAT_D16_UNORM:
        return true;
    default: return false;
    }
}

DXGI_FORMAT DirectX12Texture::GetTypelessFormat(DXGI_FORMAT format) {
    DXGI_FORMAT t = format;
    switch (format) {
    case DXGI_FORMAT_R32G32B32A32_FLOAT: case DXGI_FORMAT_R32G32B32A32_UINT: case DXGI_FORMAT_R32G32B32A32_SINT: t = DXGI_FORMAT_R32G32B32A32_TYPELESS; break;
    case DXGI_FORMAT_R32G32B32_FLOAT: case DXGI_FORMAT_R32G32B32_UINT: case DXGI_FORMAT_R32G32B32_SINT: t = DXGI_FORMAT_R32G32B32_TYPELESS; break;
    case DXGI_FORMAT_R16G16B16A16_FLOAT: case DXGI_FORMAT_R16G16B16A16_UNORM: case DXGI_FORMAT_R16G16B16A16_UINT: case DXGI_FORMAT_R16G16B16A16_SNORM: case DXGI_FORMAT_R16G16B16A16_SINT: t = DXGI_FORMAT_R16G16B16A16_TYPELESS; break;
    case DXGI_FORMAT_R32G32_FLOAT: case DXGI_FORMAT_R32G32_UINT: case DXGI_FORMAT_R32G32_SINT: t = DXGI_FORMAT_R32G32_TYPELESS; break;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: t = DXGI_FORMAT_R32G8X24_TYPELESS; break;
    case DXGI_FORMAT_R10G10B10A2_UNORM: case DXGI_FORMAT_R10G10B10A2_UINT: t = DXGI_FORMAT_R10G10B10A2_TYPELESS; break;
    case DXGI_FORMAT_R8G8B8A8_UNORM: case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: case DXGI_FORMAT_R8G8B8A8_UINT: case DXGI_FORMAT_R8G8B8A8_SNORM: case DXGI_FORMAT_R8G8B8A8_SINT: t = DXGI_FORMAT_R8G8B8A8_TYPELESS; break;
    case DXGI_FORMAT_R16G16_FLOAT: case DXGI_FORMAT_R16G16_UNORM: case DXGI_FORMAT_R16G16_UINT: case DXGI_FORMAT_R16G16_SNORM: case DXGI_FORMAT_R16G16_SINT: t = DXGI_FORMAT_R16G16_TYPELESS; break;
    case DXGI_FORMAT_D32_FLOAT: case DXGI_FORMAT_R32_FLOAT: case DXGI_FORMAT_R32_UINT: case DXGI_FORMAT_R32_SINT: t = DXGI_FORMAT_R32_TYPELESS; break;
    case DXGI_FORMAT_R8G8_UNORM: case DXGI_FORMAT_R8G8_UINT: case DXGI_FORMAT_R8G8_SNORM: case DXGI_FORMAT_R8G8_SINT: t = DXGI_FORMAT_R8G8_TYPELESS; break;
    case DXGI_FORMAT_R16_FLOAT: case DXGI_FORMAT_D16_UNORM: case DXGI_FORMAT_R16_UNORM: case DXGI_FORMAT_R16_UINT: case DXGI_FORMAT_R16_SNORM: case DXGI_FORMAT_R16_SINT: t = DXGI_FORMAT_R16_TYPELESS; break;
    case DXGI_FORMAT_R8_UNORM: case DXGI_FORMAT_R8_UINT: case DXGI_FORMAT_R8_SNORM: case DXGI_FORMAT_R8_SINT: t = DXGI_FORMAT_R8_TYPELESS; break;
    case DXGI_FORMAT_BC1_UNORM: case DXGI_FORMAT_BC1_UNORM_SRGB: t = DXGI_FORMAT_BC1_TYPELESS; break;
    case DXGI_FORMAT_BC2_UNORM: case DXGI_FORMAT_BC2_UNORM_SRGB: t = DXGI_FORMAT_BC2_TYPELESS; break;
    case DXGI_FORMAT_BC3_UNORM: case DXGI_FORMAT_BC3_UNORM_SRGB: t = DXGI_FORMAT_BC3_TYPELESS; break;
    case DXGI_FORMAT_BC4_UNORM: case DXGI_FORMAT_BC4_SNORM: t = DXGI_FORMAT_BC4_TYPELESS; break;
    case DXGI_FORMAT_BC5_UNORM: case DXGI_FORMAT_BC5_SNORM: t = DXGI_FORMAT_BC5_TYPELESS; break;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: t = DXGI_FORMAT_B8G8R8A8_TYPELESS; break;
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: t = DXGI_FORMAT_B8G8R8X8_TYPELESS; break;
    case DXGI_FORMAT_BC6H_UF16: case DXGI_FORMAT_BC6H_SF16: t = DXGI_FORMAT_BC6H_TYPELESS; break;
    case DXGI_FORMAT_BC7_UNORM: case DXGI_FORMAT_BC7_UNORM_SRGB: t = DXGI_FORMAT_BC7_TYPELESS; break;
    }
    return t;
}

DXGI_FORMAT DirectX12Texture::GetSRGBFormat(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case DXGI_FORMAT_BC1_UNORM: return DXGI_FORMAT_BC1_UNORM_SRGB;
    case DXGI_FORMAT_BC2_UNORM: return DXGI_FORMAT_BC2_UNORM_SRGB;
    case DXGI_FORMAT_BC3_UNORM: return DXGI_FORMAT_BC3_UNORM_SRGB;
    case DXGI_FORMAT_B8G8R8A8_UNORM: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    case DXGI_FORMAT_B8G8R8X8_UNORM: return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
    case DXGI_FORMAT_BC7_UNORM: return DXGI_FORMAT_BC7_UNORM_SRGB;
    }
    return format;
}

DXGI_FORMAT DirectX12Texture::GetUAVCompatableFormat(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_R8G8B8A8_TYPELESS: case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: case DXGI_FORMAT_B8G8R8A8_UNORM: case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS: case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: case DXGI_FORMAT_B8G8R8X8_TYPELESS: case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_R32_TYPELESS: case DXGI_FORMAT_D32_FLOAT: return DXGI_FORMAT_R32_FLOAT;
    }
    return format;
}
