#include "Runtime/Graphics/DefaultTextures.h"

#include <d3dx12.h>
#include <cstdint>

#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"

using namespace DeltaEngine;

namespace
{
    std::shared_ptr<DirectX12Texture> g_whiteTexture;
}

void DefaultTextures::Initialize(Device& device, CommandList& commandList)
{
    if (g_whiteTexture)
        return;

    auto desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1u, 1u, 1u, 1u);
    g_whiteTexture = device.CreateTexture(desc, nullptr);
    g_whiteTexture->SetName(L"DefaultWhiteTexture");

    static const uint32_t whitePixel = 0xFFFFFFFFu;
    D3D12_SUBRESOURCE_DATA subresource {};
    subresource.pData      = &whitePixel;
    subresource.RowPitch   = sizeof(whitePixel);
    subresource.SlicePitch = sizeof(whitePixel);

    commandList.CopyTextureSubresource(g_whiteTexture, 0u, 1u, &subresource);
    commandList.TransitionBarrier(g_whiteTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

D3D12_CPU_DESCRIPTOR_HANDLE DefaultTextures::GetWhiteSRV()
{
    if (!g_whiteTexture)
        return D3D12_CPU_DESCRIPTOR_HANDLE {};
    return g_whiteTexture->GetShaderResourceView();
}

std::shared_ptr<DirectX12Texture> DefaultTextures::GetWhiteTexture()
{
    return g_whiteTexture;
}

void DefaultTextures::Shutdown()
{
    g_whiteTexture.reset();
}
