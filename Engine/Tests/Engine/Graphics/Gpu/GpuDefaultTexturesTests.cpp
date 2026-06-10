#include "Shared/GpuGraphicsFixture.h"

#include "Runtime/Graphics/DefaultTextures.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

namespace
{
void ExpectTextureDimensions(const std::shared_ptr<DirectX12Texture>& texture,
    uint32_t width,
    uint32_t height,
    uint32_t depthOrArraySize,
    DXGI_FORMAT format)
{
    ASSERT_NE(texture, nullptr);
    const D3D12_RESOURCE_DESC desc = texture->GetD3D12ResourceDesc();
    EXPECT_EQ(desc.Width, static_cast<UINT64>(width));
    EXPECT_EQ(desc.Height, height);
    EXPECT_EQ(desc.DepthOrArraySize, depthOrArraySize);
    EXPECT_EQ(desc.Format, format);
}
}

class GpuDefaultTexturesTests : public GpuGraphicsFixture
{
protected:
    void SetUp() override
    {
        DefaultTextures::Shutdown();
    }

    void TearDown() override
    {
        DefaultTextures::Shutdown();
        GpuGraphicsFixture::TearDown();
    }
};

TEST_F(GpuDefaultTexturesTests, InitializeCreatesCoreFallbackTexturesWithValidSrvs)
{
    auto commandList = GetDirectQueue().GetCommandList();
    ASSERT_NE(commandList, nullptr);

    DefaultTextures::Initialize(*GetDevice(), *commandList);
    SubmitAndWait(commandList);

    EXPECT_NE(DefaultTextures::GetWhiteTexture(), nullptr);
    EXPECT_NE(DefaultTextures::GetWhiteSRV().ptr, 0u);
    EXPECT_NE(DefaultTextures::GetBlackCubeTexture(), nullptr);
    EXPECT_NE(DefaultTextures::GetBlackCubeSRV().ptr, 0u);
    EXPECT_NE(DefaultTextures::GetBlackRGTexture(), nullptr);
    EXPECT_NE(DefaultTextures::GetBlackRGSRV().ptr, 0u);
    EXPECT_NE(DefaultTextures::GetShadowMap2DFallback(), nullptr);
    EXPECT_NE(DefaultTextures::GetShadowMap2DFallbackSRV().ptr, 0u);
    EXPECT_NE(DefaultTextures::GetShadowCubeArrayFallback(), nullptr);
    EXPECT_NE(DefaultTextures::GetShadowCubeArrayFallbackSRV().ptr, 0u);

    ExpectTextureDimensions(
        DefaultTextures::GetWhiteTexture(), 1u, 1u, 1u, DXGI_FORMAT_R8G8B8A8_UNORM);
    ExpectTextureDimensions(
        DefaultTextures::GetBlackCubeTexture(), 1u, 1u, 6u, DXGI_FORMAT_R16G16B16A16_FLOAT);
    ExpectTextureDimensions(
        DefaultTextures::GetBlackRGTexture(), 1u, 1u, 1u, DXGI_FORMAT_R16G16B16A16_FLOAT);

    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuDefaultTexturesTests, InitializeIsIdempotent)
{
    auto commandList = GetDirectQueue().GetCommandList();
    ASSERT_NE(commandList, nullptr);

    DefaultTextures::Initialize(*GetDevice(), *commandList);
    SubmitAndWait(commandList);

    const auto whiteTexture = DefaultTextures::GetWhiteTexture();
    const auto blackCubeTexture = DefaultTextures::GetBlackCubeTexture();
    const auto blackRGTexture = DefaultTextures::GetBlackRGTexture();

    DefaultTextures::Initialize(*GetDevice(), *commandList);

    EXPECT_EQ(DefaultTextures::GetWhiteTexture(), whiteTexture);
    EXPECT_EQ(DefaultTextures::GetBlackCubeTexture(), blackCubeTexture);
    EXPECT_EQ(DefaultTextures::GetBlackRGTexture(), blackRGTexture);
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuDefaultTexturesTests, ShutdownClearsTexturesAndAllowsReinitialize)
{
    auto commandList = GetDirectQueue().GetCommandList();
    ASSERT_NE(commandList, nullptr);

    DefaultTextures::Initialize(*GetDevice(), *commandList);
    SubmitAndWait(commandList);

    DefaultTextures::Shutdown();

    EXPECT_EQ(DefaultTextures::GetWhiteTexture(), nullptr);
    EXPECT_EQ(DefaultTextures::GetWhiteSRV().ptr, 0u);
    EXPECT_EQ(DefaultTextures::GetBlackCubeTexture(), nullptr);
    EXPECT_EQ(DefaultTextures::GetBlackCubeSRV().ptr, 0u);
    EXPECT_EQ(DefaultTextures::GetBlackRGTexture(), nullptr);
    EXPECT_EQ(DefaultTextures::GetBlackRGSRV().ptr, 0u);
    EXPECT_EQ(DefaultTextures::GetShadowMap2DFallback(), nullptr);
    EXPECT_EQ(DefaultTextures::GetShadowMap2DFallbackSRV().ptr, 0u);
    EXPECT_EQ(DefaultTextures::GetShadowCubeArrayFallback(), nullptr);
    EXPECT_EQ(DefaultTextures::GetShadowCubeArrayFallbackSRV().ptr, 0u);

    DefaultTextures::Initialize(*GetDevice(), *commandList);
    SubmitAndWait(commandList);

    EXPECT_NE(DefaultTextures::GetWhiteTexture(), nullptr);
    EXPECT_NE(DefaultTextures::GetWhiteSRV().ptr, 0u);
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}
