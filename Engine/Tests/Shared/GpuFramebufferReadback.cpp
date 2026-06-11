#include "Shared/GpuFramebufferReadback.h"

#include "Shared/GpuD3D12Validation.h"

#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/CommandQueue.h"
#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"

#include <DirectXMath.h>
#include <d3dx12.h>
#include <wrl.h>

#include <cmath>
#include <cstdint>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace DeltaEngine::Tests
{
namespace
{
float HalfToFloat(const uint16_t value)
{
    return DirectX::XMConvertHalfToFloat(value);
}
} // namespace

bool TextureHasNonClearContent(Device& device,
    CommandQueue& queue,
    const std::shared_ptr<DirectX12Texture>& texture,
    const float clearR,
    const float clearG,
    const float clearB,
    const float epsilon)
{
    if (!texture)
        return false;

    ComPtr<ID3D12Resource> sourceResource = texture->GetD3D12Resource();
    if (!sourceResource)
        return false;

    const D3D12_RESOURCE_DESC sourceDesc = sourceResource->GetDesc();
    if (sourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D)
        return false;

    ID3D12Device* d3d12Device = device.GetD3D12Device().Get();
    if (!d3d12Device)
        return false;

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    UINT rowCount = 0;
    UINT64 rowSize = 0;
    UINT64 totalBytes = 0;
    d3d12Device->GetCopyableFootprints(&sourceDesc, 0, 1, 0, &footprint, &rowCount, &rowSize, &totalBytes);

    const auto readbackDesc = CD3DX12_RESOURCE_DESC::Buffer(totalBytes);
    const CD3DX12_HEAP_PROPERTIES readbackHeap(D3D12_HEAP_TYPE_READBACK);

    ComPtr<ID3D12Resource> readbackResource;
    if (FAILED(d3d12Device->CreateCommittedResource(
            &readbackHeap,
            D3D12_HEAP_FLAG_NONE,
            &readbackDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&readbackResource))))
    {
        return false;
    }

    std::shared_ptr<CommandList> commandList = queue.GetCommandList();
    if (!commandList)
        return false;

    commandList->TransitionBarrier(texture, D3D12_RESOURCE_STATE_COPY_SOURCE, 0, true);

    D3D12_TEXTURE_COPY_LOCATION dstLocation{};
    dstLocation.pResource = readbackResource.Get();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dstLocation.PlacedFootprint = footprint;

    D3D12_TEXTURE_COPY_LOCATION srcLocation{};
    srcLocation.pResource = sourceResource.Get();
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLocation.SubresourceIndex = 0;

    commandList->GetD3D12CommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

    const uint64_t fenceValue = queue.ExecuteCommandList(commandList);
    device.SetFrameFenceValue(fenceValue);
    queue.WaitForFenceValue(fenceValue);
    AssertGpuValidationClean(d3d12Device);

    D3D12_RANGE readRange{};
    readRange.Begin = 0;
    readRange.End = static_cast<SIZE_T>(totalBytes);

    uint8_t* mappedData = nullptr;
    if (FAILED(readbackResource->Map(0, &readRange, reinterpret_cast<void**>(&mappedData))) || !mappedData)
        return false;

    bool hasNonClearContent = false;
    const uint32_t height = footprint.Footprint.Height;
    const uint32_t width = footprint.Footprint.Width;
    const uint32_t rowPitch = footprint.Footprint.RowPitch;
    const DXGI_FORMAT format = footprint.Footprint.Format;

    if (format == DXGI_FORMAT_R16G16B16A16_FLOAT)
    {
        for (uint32_t y = 0; y < height && !hasNonClearContent; ++y)
        {
            const auto* row = reinterpret_cast<const uint16_t*>(mappedData + static_cast<size_t>(y) * rowPitch);
            for (uint32_t x = 0; x < width; ++x)
            {
                const uint32_t pixelIndex = x * 4u;
                const float r = HalfToFloat(row[pixelIndex + 0]);
                const float g = HalfToFloat(row[pixelIndex + 1]);
                const float b = HalfToFloat(row[pixelIndex + 2]);

                if (std::fabs(r - clearR) > epsilon || std::fabs(g - clearG) > epsilon || std::fabs(b - clearB) > epsilon)
                {
                    hasNonClearContent = true;
                    break;
                }
            }
        }
    }
    else if (format == DXGI_FORMAT_R8G8B8A8_UNORM || format == DXGI_FORMAT_B8G8R8A8_UNORM)
    {
        const float scale = 1.0f / 255.0f;
        for (uint32_t y = 0; y < height && !hasNonClearContent; ++y)
        {
            const auto* row = mappedData + static_cast<size_t>(y) * rowPitch;
            for (uint32_t x = 0; x < width; ++x)
            {
                const uint32_t pixelIndex = x * 4u;
                const float r = static_cast<float>(row[pixelIndex + 0]) * scale;
                const float g = static_cast<float>(row[pixelIndex + 1]) * scale;
                const float b = static_cast<float>(row[pixelIndex + 2]) * scale;

                if (std::fabs(r - clearR) > epsilon || std::fabs(g - clearG) > epsilon || std::fabs(b - clearB) > epsilon)
                {
                    hasNonClearContent = true;
                    break;
                }
            }
        }
    }

    readbackResource->Unmap(0, nullptr);
    return hasNonClearContent;
}

} // namespace DeltaEngine::Tests
