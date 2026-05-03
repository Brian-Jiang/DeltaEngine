#include "Runtime/Graphics/RenderProxy/DirectionalLightRenderProxy.h"
#include "Runtime/Graphics/RenderProxy/PointLightRenderProxy.h"
#include "Runtime/Graphics/RenderProxy/SpotLightRenderProxy.h"

#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/Shadow/ShadowView.h"

#include <DirectXMath.h>
#include <gtest/gtest.h>

#include <memory>
#include <vector>

using namespace DeltaEngine;
using namespace DirectX;

namespace
{
ShadowAllocation MakeAlloc()
{
    ShadowAllocation a {};
    a.atlasUVRect = { 0.0f, 0.0f, 1.0f, 1.0f };
    a.cubeArrayIndex = 0;
    return a;
}
}

TEST(LightProxyHardeningTests, Directional_WriteShadowParams_NullCtx_IsNoop)
{
    // Arrange
    DirectionalLightRenderProxy p;
    p.SetDirectionalLightBufferIndex(0);

    // Act / Assert (no crash)
    p.WriteShadowParams(nullptr, MakeAlloc());
    SUCCEED();
}

TEST(LightProxyHardeningTests, Directional_WriteShadowParams_OutOfRangeIndex_DoesNotMutateBuffer)
{
    // Arrange
    auto ctx = std::make_shared<DXGraphicsContext>();
    DirectionalLightRenderProxy p;
    p.SetDirectionalLightBufferIndex(7);

    // Act
    p.WriteShadowParams(ctx, MakeAlloc());

    // Assert: empty buffer is unchanged
    EXPECT_TRUE(ctx->directionalLights.empty());
}

TEST(LightProxyHardeningTests, Directional_GatherShadowViews_NullCtx_IsNoop)
{
    // Arrange
    DirectionalLightRenderProxy p;
    p.UpdateParameters(XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f), 1.0f, true,
        0.005f, 0.05f, 200.0f, 1.0f, 1024, 0.0f, 1.0f, 5000.0f);
    std::vector<ShadowView> views;

    // Act
    p.GatherShadowViews(nullptr, views);

    // Assert
    EXPECT_TRUE(views.empty());
}

TEST(LightProxyHardeningTests, Directional_GatherShadowViews_NoCameraInfo_IsNoop)
{
    // Arrange: ctx has neither activeRenderCamera nor camera populated
    auto ctx = std::make_shared<DXGraphicsContext>();
    DirectionalLightRenderProxy p;
    p.UpdateParameters(XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f), 1.0f, true,
        0.005f, 0.05f, 200.0f, 1.0f, 1024, 0.0f, 1.0f, 5000.0f);
    std::vector<ShadowView> views;

    // Act
    p.GatherShadowViews(ctx, views);

    // Assert
    EXPECT_TRUE(views.empty());
}

TEST(LightProxyHardeningTests, Point_WriteShadowParams_NullCtx_IsNoop)
{
    // Arrange
    PointLightRenderProxy p;

    // Act / Assert
    p.WriteShadowParams(nullptr, MakeAlloc());
    SUCCEED();
}

TEST(LightProxyHardeningTests, Point_WriteShadowParams_OutOfRangeIndex_DoesNotMutateBuffer)
{
    // Arrange
    auto ctx = std::make_shared<DXGraphicsContext>();
    PointLightRenderProxy p;
    p.SetPointLightBufferIndex(5);

    // Act
    p.WriteShadowParams(ctx, MakeAlloc());

    // Assert
    EXPECT_TRUE(ctx->pointLights.empty());
}

TEST(LightProxyHardeningTests, Point_GatherShadowViews_NullCtx_IsNoop)
{
    // Arrange
    PointLightRenderProxy p;
    p.UpdateParameters(XMVectorSet(0.f, 0.f, 0.f, 1.f), XMVectorSet(1.f, 1.f, 1.f, 0.f),
        1.0f, 10.0f, true, 0.005f, 0.05f, 0.0f, 1.0f);
    std::vector<ShadowView> views;

    // Act
    p.GatherShadowViews(nullptr, views);

    // Assert
    EXPECT_TRUE(views.empty());
}

TEST(LightProxyHardeningTests, Point_GatherShadowViews_RangeZero_IsNoop)
{
    // Arrange
    auto ctx = std::make_shared<DXGraphicsContext>();
    PointLightRenderProxy p;
    p.UpdateParameters(XMVectorSet(0.f, 0.f, 0.f, 1.f), XMVectorSet(1.f, 1.f, 1.f, 0.f),
        1.0f, 0.0f, true, 0.005f, 0.05f, 0.0f, 1.0f);
    std::vector<ShadowView> views;

    // Act
    p.GatherShadowViews(ctx, views);

    // Assert
    EXPECT_TRUE(views.empty());
}

TEST(LightProxyHardeningTests, Spot_WriteShadowParams_NullCtx_IsNoop)
{
    // Arrange
    SpotLightRenderProxy p;

    // Act / Assert
    p.WriteShadowParams(nullptr, MakeAlloc());
    SUCCEED();
}

TEST(LightProxyHardeningTests, Spot_WriteShadowParams_OutOfRangeIndex_DoesNotMutateBuffer)
{
    // Arrange
    auto ctx = std::make_shared<DXGraphicsContext>();
    SpotLightRenderProxy p;
    p.SetSpotLightBufferIndex(3);

    // Act
    p.WriteShadowParams(ctx, MakeAlloc());

    // Assert
    EXPECT_TRUE(ctx->spotLights.empty());
}

TEST(LightProxyHardeningTests, Spot_UpdateParameters_NegativeOuterCone_LeavesPreviousValues)
{
    // Arrange: first set valid params, then attempt invalid update
    SpotLightRenderProxy p;
    p.UpdateParameters(XMVectorSet(0.f, 0.f, 0.f, 1.f), XMVectorSet(0.f, -1.f, 0.f, 0.f),
        XMVectorSet(1.f, 1.f, 1.f, 0.f), 1.0f, 25.0f, 0.2f, 0.6f, true, 0.005f, 0.05f, 0.0f, 1.0f);

    // Act: invalid outerConeAngle should be rejected
    p.UpdateParameters(XMVectorSet(0.f, 0.f, 0.f, 1.f), XMVectorSet(0.f, -1.f, 0.f, 0.f),
        XMVectorSet(1.f, 1.f, 1.f, 0.f), 1.0f, 25.0f, 0.0f, -0.5f, true, 0.005f, 0.05f, 0.0f, 1.0f);

    // Assert: shadow view still produced from the previous valid state
    auto ctx = std::make_shared<DXGraphicsContext>();
    std::vector<ShadowView> views;
    p.GatherShadowViews(ctx, views);
    EXPECT_EQ(views.size(), 1u);
}

TEST(LightProxyHardeningTests, Spot_GatherShadowViews_NullCtx_IsNoop)
{
    // Arrange
    SpotLightRenderProxy p;
    p.UpdateParameters(XMVectorSet(0.f, 3.f, 0.f, 1.f), XMVectorSet(0.f, -1.f, 0.f, 0.f),
        XMVectorSet(1.f, 1.f, 1.f, 0.f), 1.0f, 25.0f, 0.2f, 0.6f, true, 0.005f, 0.05f, 0.0f, 1.0f);
    std::vector<ShadowView> views;

    // Act
    p.GatherShadowViews(nullptr, views);

    // Assert
    EXPECT_TRUE(views.empty());
}
