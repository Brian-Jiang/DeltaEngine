#include "Runtime/Graphics/DXGraphicsContext.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(DXGraphicsContextHardeningTests, DXGraphicsContext_DefaultConstruction_HasEmptyLightVectors)
{
    // Arrange / Act
    DXGraphicsContext ctx;

    // Assert
    EXPECT_TRUE(ctx.directionalLights.empty());
    EXPECT_TRUE(ctx.pointLights.empty());
    EXPECT_TRUE(ctx.spotLights.empty());
    EXPECT_EQ(ctx.camera, nullptr);
    EXPECT_FALSE(ctx.activeRenderCamera.has_value());
    EXPECT_EQ(ctx.commandList, nullptr);
}

TEST(DXGraphicsContextHardeningTests, ApplyLightBuffersToCommandList_NullCommandList_IsNoop)
{
    // Arrange
    DXGraphicsContext ctx;
    ASSERT_EQ(ctx.commandList, nullptr);

    // Act / Assert (no crash)
    ctx.ApplyLightBuffersToCommandList();
    SUCCEED();
}
