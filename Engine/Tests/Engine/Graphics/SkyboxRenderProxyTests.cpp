#include "Runtime/Graphics/RenderProxy/SkyboxRenderProxy.h"

#include "Runtime/Graphics/DXGraphicsContext.h"

#include <gtest/gtest.h>

#include <memory>

using namespace DeltaEngine;

TEST(SkyboxRenderProxyTests, SkyboxRenderProxy_DefaultConstruction_GpuCubemapIsNull)
{
    // Arrange / Act
    SkyboxRenderProxy proxy(nullptr, nullptr);

    // Assert
    EXPECT_EQ(proxy.GetGpuCubemap(), nullptr);
}

TEST(SkyboxRenderProxyTests, SkyboxRenderProxy_InitializeWithNullSources_DoesNotCrash)
{
    // Arrange
    SkyboxRenderProxy proxy(nullptr, nullptr);
    auto ctx = std::make_shared<DXGraphicsContext>();

    // Act
    proxy.Initialize(ctx);

    // Assert (ENSURE fires + Warning logged; not initialized)
    EXPECT_EQ(proxy.GetGpuCubemap(), nullptr);
}

TEST(SkyboxRenderProxyTests, SkyboxRenderProxy_InitializeWithNullCtx_DoesNotCrash)
{
    // Arrange
    SkyboxRenderProxy proxy(nullptr, nullptr);

    // Act
    proxy.Initialize(nullptr);

    // Assert
    EXPECT_EQ(proxy.GetGpuCubemap(), nullptr);
}

TEST(SkyboxRenderProxyTests, SkyboxRenderProxy_GatherDrawCallsBeforeInitialize_IsNoop)
{
    // Arrange
    SkyboxRenderProxy proxy(nullptr, nullptr);
    auto ctx = std::make_shared<DXGraphicsContext>();

    // Act / Assert (no crash)
    proxy.GatherDrawCalls(ctx);
    SUCCEED();
}
