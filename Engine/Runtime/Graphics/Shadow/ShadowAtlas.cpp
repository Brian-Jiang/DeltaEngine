#include "Runtime/Graphics/Shadow/ShadowAtlas.h"

#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"

#include <d3dx12.h>

using namespace DeltaEngine;

namespace
{
std::wstring Utf8DebugNameToWide(const std::string& utf8)
{
    if (utf8.empty())
        return {};

    const int size = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS,
        utf8.data(), static_cast<int>(utf8.size()),
        nullptr, 0);

    if (size <= 0)
        return {};

    std::wstring wide(static_cast<size_t>(size), L'\0');
    const int written = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS,
        utf8.data(), static_cast<int>(utf8.size()),
        wide.data(), size);
    if (written != size)
        return {};
    return wide;
}
}

void ShadowAtlas::Initialize(Device& device, uint32_t widthHeight, const std::string& debugNameUtf8)
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
    if (!m_texture)
    {
        DLOG(LogShadow, ELogLevel::Error,
            "ShadowAtlas::Initialize failed: CreateTexture returned nullptr (size={}x{}, debugName='{}', expected valid depth texture)",
            m_size, m_size, debugNameUtf8);
        m_size = 0;
        return;
    }

    const std::wstring wideName = Utf8DebugNameToWide(debugNameUtf8);
    if (!wideName.empty())
        m_texture->SetName(wideName.c_str());
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
