#include "Graphics/Shadow/ShadowCubeArray.h"

#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"

#include <d3dx12.h>

using namespace DeltaEngine;

void ShadowCubeArray::Initialize(Device& device, uint32_t faceSize, uint32_t cubeCount, const wchar_t* debugName)
{
    Shutdown();
    m_faceSize = std::max(1u, faceSize);
    m_cubeCount = std::max(1u, cubeCount);
    const UINT arraySlices = static_cast<UINT>(m_cubeCount * 6u);

    auto desc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R32_TYPELESS,
        m_faceSize, m_faceSize,
        arraySlices, 1u, 1u, 0,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil = { 1.0f, 0 };

    m_texture = device.CreateTexture(desc, &clearValue);
    if (debugName && m_texture)
        m_texture->SetName(debugName);

    if (m_texture)
        m_texture->CreateTextureCubeArraySRV();

    m_faceDSVs = device.AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, arraySlices);
    auto d3d = device.GetD3D12Device().Get();
    for (UINT flat = 0; flat < arraySlices; ++flat) {
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        dsvDesc.Texture2DArray.MipSlice = 0;
        dsvDesc.Texture2DArray.FirstArraySlice = flat;
        dsvDesc.Texture2DArray.ArraySize = 1;
        d3d->CreateDepthStencilView(m_texture->GetD3D12Resource().Get(), &dsvDesc,
            m_faceDSVs.GetDescriptorHandle(flat));
    }
}

void ShadowCubeArray::Shutdown()
{
    m_faceDSVs = DescriptorAllocation();
    m_texture.reset();
    m_faceSize = 0;
    m_cubeCount = 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE ShadowCubeArray::GetDSVForFace(uint32_t cubeIndex, uint32_t face) const
{
    if (!m_faceDSVs.IsValid() || cubeIndex >= m_cubeCount || face >= 6u)
        return {};
    const UINT flat = cubeIndex * 6u + face;
    return m_faceDSVs.GetDescriptorHandle(flat);
}

D3D12_CPU_DESCRIPTOR_HANDLE ShadowCubeArray::GetFullDSV() const
{
    if (!m_texture)
        return {};
    return m_texture->GetDepthStencilView();
}

D3D12_CPU_DESCRIPTOR_HANDLE ShadowCubeArray::GetSRV() const
{
    if (!m_texture)
        return {};
    return m_texture->GetShaderResourceView();
}
