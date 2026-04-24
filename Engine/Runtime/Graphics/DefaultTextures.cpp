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
    std::shared_ptr<DirectX12Texture> g_blackCubeTexture;
    std::shared_ptr<DirectX12Texture> g_blackRGTexture;
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

    {
        auto cubeDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT,
            1u, 1u, 6u, 1u);
        g_blackCubeTexture = device.CreateTexture(cubeDesc, nullptr);
        g_blackCubeTexture->SetName(L"DefaultBlackCubeTexture");
        g_blackCubeTexture->CreateCubemapSRV();

        static const uint16_t blackCubeTexel[4] = { 0, 0, 0, 0 };
        D3D12_SUBRESOURCE_DATA faceData[6];
        for (uint32_t f = 0; f < 6; ++f)
        {
            faceData[f].pData      = blackCubeTexel;
            faceData[f].RowPitch   = sizeof(blackCubeTexel);
            faceData[f].SlicePitch = sizeof(blackCubeTexel);
        }
        commandList.CopyTextureSubresource(g_blackCubeTexture, 0u, 6u, faceData);
        commandList.TransitionBarrier(g_blackCubeTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    {
        auto rgDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT,
            1u, 1u, 1u, 1u);
        g_blackRGTexture = device.CreateTexture(rgDesc, nullptr);
        g_blackRGTexture->SetName(L"DefaultBlackRGTexture");

        static const uint16_t blackRGTexel[4] = { 0, 0, 0, 0 };
        D3D12_SUBRESOURCE_DATA rgData{};
        rgData.pData      = blackRGTexel;
        rgData.RowPitch   = sizeof(blackRGTexel);
        rgData.SlicePitch = sizeof(blackRGTexel);
        commandList.CopyTextureSubresource(g_blackRGTexture, 0u, 1u, &rgData);
        commandList.TransitionBarrier(g_blackRGTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
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

std::shared_ptr<DirectX12Texture> DefaultTextures::GetBlackCubeTexture()
{
    return g_blackCubeTexture;
}

D3D12_CPU_DESCRIPTOR_HANDLE DefaultTextures::GetBlackCubeSRV()
{
    if (!g_blackCubeTexture)
        return D3D12_CPU_DESCRIPTOR_HANDLE{};
    return g_blackCubeTexture->GetShaderResourceView();
}

std::shared_ptr<DirectX12Texture> DefaultTextures::GetBlackRGTexture()
{
    return g_blackRGTexture;
}

D3D12_CPU_DESCRIPTOR_HANDLE DefaultTextures::GetBlackRGSRV()
{
    if (!g_blackRGTexture)
        return D3D12_CPU_DESCRIPTOR_HANDLE{};
    return g_blackRGTexture->GetShaderResourceView();
}

void DefaultTextures::Shutdown()
{
    g_whiteTexture.reset();
    g_blackCubeTexture.reset();
    g_blackRGTexture.reset();
}
