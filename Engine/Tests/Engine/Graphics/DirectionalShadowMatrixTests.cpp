#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/RenderProxy/DirectionalLightRenderProxy.h"
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
bool MatricesNearEqual(FXMMATRIX a, FXMMATRIX b, float eps)
{
    XMMATRIX ma = a;
    XMMATRIX mb = b;
    for (uint32_t i = 0; i < 4; ++i)
    {
        const XMVECTOR d = XMVectorSubtract(ma.r[i], mb.r[i]);
        const XMVECTOR ad = XMVectorAbs(d);
        if (XMVectorGetX(ad) > eps || XMVectorGetY(ad) > eps || XMVectorGetZ(ad) > eps || XMVectorGetW(ad) > eps)
            return false;
    }
    return true;
}

std::shared_ptr<DXGraphicsContext> MakeCtx(const ActiveRenderCamera& arc)
{
    auto ctx = std::make_shared<DXGraphicsContext>();
    ctx->activeRenderCamera = arc;
    ctx->shadowQualityScalar = 1.0f;
    return ctx;
}

void FillArc(ActiveRenderCamera& arc)
{
    arc.nearPlane = 0.5f;
    arc.farPlane = 80.0f;
    arc.fovY = XM_PIDIV4;
    arc.aspectRatio = 1.0f;
    arc.cb.viewMatrix = XMMatrixIdentity();
    arc.cb.projectionMatrix = XMMatrixPerspectiveFovLH(arc.fovY, arc.aspectRatio, arc.nearPlane, arc.farPlane);
    arc.cb.position = XMVectorSet(0.0f, 2.0f, -8.0f, 1.0f);
}
}

TEST(DirectionalShadowMatrixTests, LargerOrthoPaddingChangesViewProj)
{
    ActiveRenderCamera arc {};
    FillArc(arc);

    DirectionalLightRenderProxy lo;
    lo.SetDirectionalLightBufferIndex(0);
    lo.UpdateParameters(XMVectorSet(0.2f, -0.8f, 0.1f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f), 1.0f, true, 0.005f,
        0.05f, 120.0f, 0.5f, 1024, 0.0f, 1.0f);

    DirectionalLightRenderProxy hi = lo;
    hi.UpdateParameters(XMVectorSet(0.2f, -0.8f, 0.1f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f), 1.0f, true, 0.005f,
        0.05f, 120.0f, 40.0f, 1024, 0.0f, 1.0f);

    auto ctxLo = MakeCtx(arc);
    std::vector<ShadowView> viewsLo;
    lo.GatherShadowViews(ctxLo, viewsLo);
    ASSERT_EQ(viewsLo.size(), 1u);
    lo.FinishShadowViewProj(viewsLo[0].shadowMapEdgePx, viewsLo[0].shadowMapEdgePx, viewsLo[0]);
    const XMMATRIX mLo = viewsLo[0].viewProj;

    auto ctxHi = MakeCtx(arc);
    std::vector<ShadowView> viewsHi;
    hi.GatherShadowViews(ctxHi, viewsHi);
    ASSERT_EQ(viewsHi.size(), 1u);
    hi.FinishShadowViewProj(viewsHi[0].shadowMapEdgePx, viewsHi[0].shadowMapEdgePx, viewsHi[0]);
    const XMMATRIX mHi = viewsHi[0].viewProj;

    EXPECT_FALSE(MatricesNearEqual(mLo, mHi, 1.0e-6f));
    const float sLo = fabsf(XMVectorGetX(mLo.r[0]));
    const float sHi = fabsf(XMVectorGetX(mHi.r[0]));
    EXPECT_GT(sLo, sHi);
}

TEST(DirectionalShadowMatrixTests, TexelSnapStabilizesSubPixelFrustumChange)
{
    ActiveRenderCamera arc0 {};
    FillArc(arc0);

    ActiveRenderCamera arc1 = arc0;
    arc1.fovY += 1.0e-6f;
    arc1.cb.projectionMatrix =
        XMMatrixPerspectiveFovLH(arc1.fovY, arc1.aspectRatio, arc1.nearPlane, arc1.farPlane);

    DirectionalLightRenderProxy proxy;
    proxy.SetDirectionalLightBufferIndex(0);
    proxy.UpdateParameters(XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f), 1.0f, true,
        0.005f, 0.05f, 100.0f, 2.0f, 1024, 0.0f, 1.0f);

    auto ctx0 = MakeCtx(arc0);
    std::vector<ShadowView> v0;
    proxy.GatherShadowViews(ctx0, v0);
    ASSERT_EQ(v0.size(), 1u);
    proxy.FinishShadowViewProj(v0[0].shadowMapEdgePx, v0[0].shadowMapEdgePx, v0[0]);
    const XMMATRIX m0 = v0[0].viewProj;

    auto ctx1 = MakeCtx(arc1);
    std::vector<ShadowView> v1;
    proxy.GatherShadowViews(ctx1, v1);
    ASSERT_EQ(v1.size(), 1u);
    proxy.FinishShadowViewProj(v1[0].shadowMapEdgePx, v1[0].shadowMapEdgePx, v1[0]);
    const XMMATRIX m1 = v1[0].viewProj;

    EXPECT_TRUE(MatricesNearEqual(m0, m1, 5.0e-5f));
}
