#include "Shared/GpuGraphicsFixture.h"

#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/IBL/IBLBaker.h"

#include <d3dx12.h>
#include <gtest/gtest.h>
#include <vector>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

namespace
{
constexpr uint32_t kTestCubemapSize = 16u;

void ExpectTextureDimensions(const std::shared_ptr<DirectX12Texture>& texture,
    uint32_t width,
    uint32_t height,
    uint32_t depthOrArraySize,
    uint32_t mipLevels,
    DXGI_FORMAT format)
{
    ASSERT_NE(texture, nullptr);
    const D3D12_RESOURCE_DESC desc = texture->GetD3D12ResourceDesc();
    EXPECT_EQ(desc.Width, static_cast<UINT64>(width));
    EXPECT_EQ(desc.Height, height);
    EXPECT_EQ(desc.DepthOrArraySize, depthOrArraySize);
    EXPECT_EQ(desc.MipLevels, mipLevels);
    EXPECT_EQ(desc.Format, format);
}

std::shared_ptr<DirectX12Texture> CreateUploadedTestCubemap(Device& device, CommandQueue& queue)
{
    const auto cubeDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        kTestCubemapSize,
        kTestCubemapSize,
        6u,
        1u);
    auto cubemap = device.CreateTexture(cubeDesc, nullptr);
    if (!cubemap)
        return nullptr;

    cubemap->SetName("GpuTestSourceCubemap");
    cubemap->CreateCubemapSRV();

    static const uint16_t kFaceTexel[4] = {
        0x3C00u, // 1.0f
        0x3800u, // 0.5f
        0x3400u, // 0.25f
        0x3C00u, // 1.0f
    };

    const uint32_t rowPitch = kTestCubemapSize * sizeof(kFaceTexel);
    const uint32_t slicePitch = rowPitch * kTestCubemapSize;
    std::vector<uint16_t> facePixels((kTestCubemapSize * kTestCubemapSize) * 4u, 0u);
    for (size_t i = 0; i < facePixels.size(); i += 4u)
    {
        facePixels[i + 0] = kFaceTexel[0];
        facePixels[i + 1] = kFaceTexel[1];
        facePixels[i + 2] = kFaceTexel[2];
        facePixels[i + 3] = kFaceTexel[3];
    }

    D3D12_SUBRESOURCE_DATA faceData[6];
    for (uint32_t face = 0; face < 6u; ++face)
    {
        faceData[face].pData = facePixels.data();
        faceData[face].RowPitch = rowPitch;
        faceData[face].SlicePitch = slicePitch;
    }

    auto commandList = queue.GetCommandList();
    if (!commandList)
        return nullptr;

    commandList->CopyTextureSubresource(cubemap, 0u, 6u, faceData);
    commandList->TransitionBarrier(cubemap,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    const uint64_t fenceValue = queue.ExecuteCommandList(commandList);
    device.SetFrameFenceValue(fenceValue);
    queue.WaitForFenceValue(fenceValue);
    return cubemap;
}
} // namespace

class GpuIBLBakerTests : public GpuGraphicsFixture
{
};

TEST_F(GpuIBLBakerTests, Initialize_ComputesStaticBrdfLut)
{
    IBLBaker baker;
    baker.Initialize(*GetDevice());

    const IBLBaker::IBLResources& staticLut = baker.GetStaticLut();
    ASSERT_NE(staticLut.brdfLut, nullptr);
    EXPECT_NE(staticLut.brdfLutSRV.ptr, 0u);
    ExpectTextureDimensions(staticLut.brdfLut, 512u, 512u, 1u, 1u, DXGI_FORMAT_R16G16B16A16_FLOAT);
    EXPECT_EQ(staticLut.irradianceCube, nullptr);
    EXPECT_EQ(staticLut.specularCube, nullptr);

    baker.Shutdown();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuIBLBakerTests, Shutdown_ReleasesStaticLut)
{
    IBLBaker baker;
    baker.Initialize(*GetDevice());
    ASSERT_NE(baker.GetStaticLut().brdfLut, nullptr);

    baker.Shutdown();

    const IBLBaker::IBLResources& staticLut = baker.GetStaticLut();
    EXPECT_EQ(staticLut.brdfLut, nullptr);
    EXPECT_EQ(staticLut.irradianceCube, nullptr);
    EXPECT_EQ(staticLut.specularCube, nullptr);
    EXPECT_EQ(staticLut.brdfLutSRV.ptr, 0u);
    EXPECT_EQ(staticLut.irradianceSRV.ptr, 0u);
    EXPECT_EQ(staticLut.specularSRV.ptr, 0u);

    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuIBLBakerTests, ReInitialize_IsSafe)
{
    IBLBaker baker;
    baker.Initialize(*GetDevice());
    ASSERT_NE(baker.GetStaticLut().brdfLut, nullptr);

    baker.Shutdown();
    EXPECT_EQ(baker.GetStaticLut().brdfLut, nullptr);

    baker.Initialize(*GetDevice());
    ASSERT_NE(baker.GetStaticLut().brdfLut, nullptr);
    EXPECT_NE(baker.GetStaticLut().brdfLutSRV.ptr, 0u);

    baker.Shutdown();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuIBLBakerTests, Bake_WithUploadedCubemap_ProducesIrradianceAndSpecular)
{
    auto sourceCube = CreateUploadedTestCubemap(*GetDevice(), GetDirectQueue());
    ASSERT_NE(sourceCube, nullptr);
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    IBLBaker baker;
    baker.Initialize(*GetDevice());
    const auto& staticLut = baker.GetStaticLut();
    ASSERT_NE(staticLut.brdfLut, nullptr);

    const IBLBaker::IBLResources resources = baker.Bake(*GetDevice(), sourceCube);
    ASSERT_NE(resources.irradianceCube, nullptr);
    ASSERT_NE(resources.specularCube, nullptr);
    ASSERT_NE(resources.brdfLut, nullptr);
    EXPECT_EQ(resources.brdfLut, staticLut.brdfLut);
    EXPECT_EQ(resources.brdfLutSRV.ptr, staticLut.brdfLutSRV.ptr);

    ExpectTextureDimensions(
        resources.irradianceCube, 32u, 32u, 6u, 1u, DXGI_FORMAT_R16G16B16A16_FLOAT);
    ExpectTextureDimensions(
        resources.specularCube, 128u, 128u, 6u, 7u, DXGI_FORMAT_R16G16B16A16_FLOAT);

    EXPECT_NE(resources.irradianceSRV.ptr, 0u);
    EXPECT_NE(resources.specularSRV.ptr, 0u);
    EXPECT_NE(resources.brdfLutSRV.ptr, 0u);

    baker.Shutdown();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}
