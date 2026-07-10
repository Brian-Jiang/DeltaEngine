#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/RenderProxy/DirectionalLightRenderProxy.h"
#include "Runtime/Graphics/RenderProxy/PointLightRenderProxy.h"
#include "Runtime/Graphics/RenderProxy/SpotLightRenderProxy.h"
#include "Runtime/Graphics/Shadow/ShadowView.h"
#include "Runtime/Graphics/Structures/Camera.h"

#include <DirectXMath.h>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

using namespace DeltaEngine;
using namespace DirectX;

namespace
{
std::shared_ptr<DXGraphicsContext> MakeDirCtx()
{
    auto ctx = std::make_shared<DXGraphicsContext>();
    ActiveRenderCamera arc {};
    arc.nearPlane = 0.1f;
    arc.farPlane = 100.0f;
    arc.fovY = XM_PIDIV4;
    arc.aspectRatio = 16.0f / 9.0f;
    arc.cb.viewMatrix = XMMatrixIdentity();
    arc.cb.projectionMatrix = XMMatrixPerspectiveFovLH(arc.fovY, arc.aspectRatio, arc.nearPlane, arc.farPlane);
    ctx->activeRenderCamera = arc;
    return ctx;
}

bool Row3NearAffineBottom(FXMMATRIX mIn, float eps)
{
    XMMATRIX m = mIn;
    const XMVECTOR r3 = m.r[3];
    return fabsf(XMVectorGetX(r3)) <= eps && fabsf(XMVectorGetY(r3)) <= eps && fabsf(XMVectorGetZ(r3)) <= eps
        && fabsf(XMVectorGetW(r3) - 1.0f) <= eps;
}
}

TEST(LightProxyShadowTests, DirectionalProducesOneViewWhenCasting)
{
    auto ctx = MakeDirCtx();
    DirectionalLightRenderProxy p;
    p.SetDirectionalLightBufferIndex(0);
    p.UpdateParameters(XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f), 1.0f, true, 0.005f,
        0.05f, 200.0f, 1.0f, 1024, 0.0f, 1.0f, 5000.0f);

    std::vector<ShadowView> views;
    p.GatherShadowViews(ctx, views);
    ASSERT_EQ(views.size(), 1u);
    EXPECT_EQ(views[0].type, LightType::Directional);
}

TEST(LightProxyShadowTests, DirectionalProducesNoViewWhenShadowsOff)
{
    auto ctx = MakeDirCtx();
    DirectionalLightRenderProxy p;
    p.UpdateParameters(XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f), 1.0f, false, 0.005f,
        0.05f, 200.0f, 1.0f, 1024, 0.0f, 1.0f, 5000.0f);

    std::vector<ShadowView> views;
    p.GatherShadowViews(ctx, views);
    EXPECT_TRUE(views.empty());
}

TEST(LightProxyShadowTests, SpotProducesOnePerspectiveView)
{
    auto ctx = std::make_shared<DXGraphicsContext>();
    SpotLightRenderProxy p;
    p.SetSpotLightBufferIndex(0);
    p.UpdateParameters(XMVectorSet(0.0f, 3.0f, 0.0f, 1.0f), XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f),
        XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f), 1.0f, 25.0f, 0.2f, 0.6f, true, 0.005f, 0.05f, 0.0f, 1.0f, 1024);

    std::vector<ShadowView> views;
    p.GatherShadowViews(ctx, views);
    ASSERT_EQ(views.size(), 1u);
    EXPECT_EQ(views[0].type, LightType::Spot);
    EXPECT_FALSE(Row3NearAffineBottom(views[0].viewProj, 1.0e-3f));
}

TEST(LightProxyShadowTests, PointProducesSixCubeFaceViews)
{
    auto ctx = std::make_shared<DXGraphicsContext>();
    PointLightRenderProxy p;
    p.SetPointLightBufferIndex(2);
    p.UpdateParameters(XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f), XMVectorSet(1.0f, 0.8f, 0.6f, 0.0f), 2.0f, 15.0f, true,
        0.005f, 0.05f, 0.0f, 1.0f);

    std::vector<ShadowView> views;
    p.GatherShadowViews(ctx, views);
    ASSERT_EQ(views.size(), 6u);
    for (uint32_t i = 0; i < 6; ++i)
    {
        EXPECT_EQ(views[i].type, LightType::Point);
        EXPECT_EQ(views[i].lightIndex, 2u);
        EXPECT_EQ(views[i].cubeFace, i);
        EXPECT_FALSE(Row3NearAffineBottom(views[i].viewProj, 1.0e-3f));
    }
}

TEST(LightProxyShadowTests, DirectionalFinishedMatrixIsAffineBottomRow)
{
    auto ctx = MakeDirCtx();
    DirectionalLightRenderProxy p;
    p.SetDirectionalLightBufferIndex(0);
    p.UpdateParameters(XMVectorSet(0.3f, -0.7f, 0.2f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f), 1.0f, true, 0.005f,
        0.05f, 150.0f, 2.0f, 1024, 0.0f, 1.0f, 5000.0f);

    std::vector<ShadowView> views;
    p.GatherShadowViews(ctx, views);
    ASSERT_EQ(views.size(), 1u);
    p.FinishShadowViewProj(1024, 1024, views[0]);

    EXPECT_TRUE(Row3NearAffineBottom(views[0].viewProj, 5.0e-4f));
}
