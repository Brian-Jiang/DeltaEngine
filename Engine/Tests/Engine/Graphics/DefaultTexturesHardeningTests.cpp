#include "Runtime/Graphics/DefaultTextures.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

class DefaultTexturesHardeningFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        DefaultTextures::Shutdown();
    }

    void TearDown() override
    {
        DefaultTextures::Shutdown();
    }
};

TEST_F(DefaultTexturesHardeningFixture, DefaultTextures_GetWhiteSRV_BeforeInitialize_ReturnsZeroHandle)
{
    // Act
    const D3D12_CPU_DESCRIPTOR_HANDLE handle = DefaultTextures::GetWhiteSRV();

    // Assert
    EXPECT_EQ(handle.ptr, 0u);
    EXPECT_EQ(DefaultTextures::GetWhiteTexture(), nullptr);
}

TEST_F(DefaultTexturesHardeningFixture, DefaultTextures_GetBlackCubeSRV_BeforeInitialize_ReturnsZeroHandle)
{
    EXPECT_EQ(DefaultTextures::GetBlackCubeSRV().ptr, 0u);
    EXPECT_EQ(DefaultTextures::GetBlackCubeTexture(), nullptr);
}

TEST_F(DefaultTexturesHardeningFixture, DefaultTextures_GetBlackRGSRV_BeforeInitialize_ReturnsZeroHandle)
{
    EXPECT_EQ(DefaultTextures::GetBlackRGSRV().ptr, 0u);
    EXPECT_EQ(DefaultTextures::GetBlackRGTexture(), nullptr);
}

TEST_F(DefaultTexturesHardeningFixture, DefaultTextures_GetShadowMap2DFallbackSRV_BeforeInitialize_ReturnsZeroHandle)
{
    EXPECT_EQ(DefaultTextures::GetShadowMap2DFallbackSRV().ptr, 0u);
    EXPECT_EQ(DefaultTextures::GetShadowMap2DFallback(), nullptr);
}

TEST_F(DefaultTexturesHardeningFixture, DefaultTextures_GetShadowCubeArrayFallbackSRV_BeforeInitialize_ReturnsZeroHandle)
{
    EXPECT_EQ(DefaultTextures::GetShadowCubeArrayFallbackSRV().ptr, 0u);
    EXPECT_EQ(DefaultTextures::GetShadowCubeArrayFallback(), nullptr);
}

TEST_F(DefaultTexturesHardeningFixture, DefaultTextures_Shutdown_BeforeInitialize_DoesNotCrash)
{
    // Act / Assert
    DefaultTextures::Shutdown();
    DefaultTextures::Shutdown();
    SUCCEED();
}
