#include "Shared/GpuGraphicsFixture.h"

#include "Runtime/Graphics/DirectX/DescriptorAllocation.h"

#include <d3dx12.h>
#include <gtest/gtest.h>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class GpuDeviceSmokeTests : public GpuGraphicsFixture
{
};

TEST_F(GpuDeviceSmokeTests, FixtureProvidesWarpDeviceAnd64x64RenderTarget)
{
    ASSERT_NE(GetDevice(), nullptr);
    ASSERT_NE(GetRenderTarget(), nullptr);
    EXPECT_EQ(GetRenderTarget()->GetWidth(), 64u);
    EXPECT_EQ(GetRenderTarget()->GetHeight(), 64u);
}

TEST_F(GpuDeviceSmokeTests, DescriptorAllocationReturnsValidHandle)
{
    auto allocation = GetDevice()->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4);
    EXPECT_TRUE(allocation.IsValid());
}

TEST_F(GpuDeviceSmokeTests, CreatesCommittedTextureAndBuffer)
{
    const auto textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
    D3D12_CLEAR_VALUE clearValue = { textureDesc.Format, { 0.0f, 0.0f, 0.0f, 1.0f } };

    auto texture = GetDevice()->CreateTexture(textureDesc, &clearValue);
    auto buffer = GetDevice()->CreateByteAddressBuffer(256);

    EXPECT_NE(texture, nullptr);
    EXPECT_NE(buffer, nullptr);
}

TEST_F(GpuDeviceSmokeTests, CommandListRecordExecuteFenceWaitLeavesQueueClean)
{
    auto commandList = GetDirectQueue().GetCommandList();
    ASSERT_NE(commandList, nullptr);

    commandList->SetRenderTarget(*GetRenderTarget());
    const float clearColor[4] = { 0.1f, 0.2f, 0.3f, 1.0f };
    commandList->ClearTexture(GetRenderTarget()->GetTexture(AttachmentPoint::Color0), clearColor);

    const uint64_t fenceValue = SubmitAndWait(commandList);
    EXPECT_TRUE(GetDirectQueue().IsFenceComplete(fenceValue));
}
