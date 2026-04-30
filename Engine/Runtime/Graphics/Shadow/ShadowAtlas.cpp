#include "Graphics/Shadow/ShadowAtlas.h"

#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"

#include <d3dx12.h>

using namespace DeltaEngine;

void ShadowAtlas::Initialize(Device& device, uint32_t widthHeight, const wchar_t* debugName)
{
    Shutdown();
    m_size = std::max(1u, widthHeight);

    auto desc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R32_TYPELESS,
        m_size, m_size,
        1u, 1u, 1u, 0,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil = { 1.0f, 0 };

    m_texture = device.CreateTexture(desc, &clearValue);
    if (debugName && m_texture)
        m_texture->SetName(debugName);
}

void ShadowAtlas::Shutdown()
{
    m_texture.reset();
    m_size = 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE ShadowAtlas::GetDSV() const
{
    if (!m_texture)
        return {};
    return m_texture->GetDepthStencilView();
}

D3D12_CPU_DESCRIPTOR_HANDLE ShadowAtlas::GetSRV() const
{
    if (!m_texture)
        return {};
    return m_texture->GetShaderResourceView();
}
