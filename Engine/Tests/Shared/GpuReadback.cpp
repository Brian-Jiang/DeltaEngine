#include "Shared/GpuReadback.h"

#include "Shared/GpuD3D12Validation.h"

#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/CommandQueue.h"
#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"

#include <DirectXPackedVector.h>
#include <d3dx12.h>
#include <wrl.h>

#include <cmath>
#include <cstdint>

using Microsoft::WRL::ComPtr;

namespace DeltaEngine::Tests
{
namespace
{
float HalfToFloat(const uint16_t value)
{
    return DirectX::PackedVector::XMConvertHalfToFloat(value);
}

bool DecodeCenterPixel(const uint8_t* mappedData,
    const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint,
    GpuReadbackPixel& outPixel)
{
    const uint32_t width = footprint.Footprint.Width;
    const uint32_t height = footprint.Footprint.Height;
    if (width == 0 || height == 0)
        return false;

    const uint32_t centerX = width / 2u;
    const uint32_t centerY = height / 2u;
    const uint32_t rowPitch = footprint.Footprint.RowPitch;
    const DXGI_FORMAT format = footprint.Footprint.Format;

    if (format == DXGI_FORMAT_R16G16B16A16_FLOAT)
    {
        const auto* row = reinterpret_cast<const uint16_t*>(mappedData + static_cast<size_t>(centerY) * rowPitch);
        const uint32_t pixelIndex = centerX * 4u;
        outPixel.r = HalfToFloat(row[pixelIndex + 0]);
        outPixel.g = HalfToFloat(row[pixelIndex + 1]);
        outPixel.b = HalfToFloat(row[pixelIndex + 2]);
        outPixel.a = HalfToFloat(row[pixelIndex + 3]);
        return true;
    }

    if (format == DXGI_FORMAT_R8G8B8A8_UNORM || format == DXGI_FORMAT_B8G8R8A8_UNORM)
    {
        const float scale = 1.0f / 255.0f;
        const auto* row = mappedData + static_cast<size_t>(centerY) * rowPitch;
        const uint32_t pixelIndex = centerX * 4u;
        outPixel.r = static_cast<float>(row[pixelIndex + 0]) * scale;
        outPixel.g = static_cast<float>(row[pixelIndex + 1]) * scale;
        outPixel.b = static_cast<float>(row[pixelIndex + 2]) * scale;
        outPixel.a = static_cast<float>(row[pixelIndex + 3]) * scale;
        return true;
    }

    return false;
}
} // namespace

bool ReadTextureCenterPixel(Device& device,
    CommandQueue& queue,
    const std::shared_ptr<DirectX12Texture>& texture,
    GpuReadbackPixel& outPixel)
{
    outPixel = {};

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

    const bool decoded = DecodeCenterPixel(mappedData, footprint, outPixel);
    readbackResource->Unmap(0, nullptr);
    return decoded;
}

bool TextureHasNonClearContent(Device& device,
    CommandQueue& queue,
    const std::shared_ptr<DirectX12Texture>& texture,
    const float clearR,
    const float clearG,
    const float clearB,
    const float epsilon)
{
    GpuReadbackPixel pixel{};
    if (!ReadTextureCenterPixel(device, queue, texture, pixel))
        return false;

    return std::fabs(pixel.r - clearR) > epsilon || std::fabs(pixel.g - clearG) > epsilon
        || std::fabs(pixel.b - clearB) > epsilon;
}

} // namespace DeltaEngine::Tests
