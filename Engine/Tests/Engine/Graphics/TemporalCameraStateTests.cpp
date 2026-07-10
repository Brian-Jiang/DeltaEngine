#include "Runtime/Graphics/Structures/TemporalCameraState.h"

#include <cmath>
#include <DirectXMath.h>
#include <gtest/gtest.h>

using namespace DeltaEngine;
using namespace DirectX;

namespace
{
bool MatrixNearEqual(FXMMATRIX a, FXMMATRIX b, float eps = 1e-5f)
{
    for (int r = 0; r < 4; ++r)
    {
        XMFLOAT4 fa, fb;
        XMStoreFloat4(&fa, a.r[r]);
        XMStoreFloat4(&fb, b.r[r]);
        if (std::abs(fa.x - fb.x) > eps || std::abs(fa.y - fb.y) > eps ||
            std::abs(fa.z - fb.z) > eps || std::abs(fa.w - fb.w) > eps)
            return false;
    }
    return true;
}
} // namespace

TEST(TemporalCameraStateTests, Halton_Base2_KnownSequence)
{
    EXPECT_FLOAT_EQ(Halton(1, 2), 0.5f);
    EXPECT_FLOAT_EQ(Halton(2, 2), 0.25f);
    EXPECT_FLOAT_EQ(Halton(3, 2), 0.75f);
}

TEST(TemporalCameraStateTests, HaltonJitterNdc_ScalesByRenderTargetSize)
{
    const XMFLOAT2 j = HaltonJitterNdc(0, 100, 200);
    // sampleIndex 0 -> Halton index 1: h2=0.5, h3=1/3
    const float expectedX = (0.5f - 0.5f) * 2.0f / 100.0f;
    const float expectedY = (Halton(1, 3) - 0.5f) * 2.0f / 200.0f;
    EXPECT_FLOAT_EQ(j.x, expectedX);
    EXPECT_FLOAT_EQ(j.y, expectedY);
}

TEST(TemporalCameraStateTests, HaltonJitterNdc_ZeroSize_ReturnsZero)
{
    const XMFLOAT2 j = HaltonJitterNdc(3, 0, 100);
    EXPECT_FLOAT_EQ(j.x, 0.f);
    EXPECT_FLOAT_EQ(j.y, 0.f);
}

TEST(TemporalCameraStateTests, ApplyProjectionJitter_OffsetsRow2XY)
{
    const XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 16.f / 9.f, 0.1f, 100.f);
    const XMFLOAT2 jitter{ 0.01f, -0.02f };
    const XMMATRIX jittered = ApplyProjectionJitter(proj, jitter);

    XMFLOAT4 row2Proj, row2Jittered;
    XMStoreFloat4(&row2Proj, proj.r[2]);
    XMStoreFloat4(&row2Jittered, jittered.r[2]);
    EXPECT_FLOAT_EQ(row2Jittered.x, row2Proj.x + jitter.x);
    EXPECT_FLOAT_EQ(row2Jittered.y, row2Proj.y + jitter.y);
    EXPECT_FLOAT_EQ(row2Jittered.z, row2Proj.z);
    EXPECT_FLOAT_EQ(row2Jittered.w, row2Proj.w);
}

TEST(TemporalCameraStateTests, HistoryReset_PrevEqualsCurrentUnjitteredVP)
{
    TemporalCameraState state;
    state.enabled = true;
    state.BeginFrame(128, 128);

    const XMMATRIX view = XMMatrixLookToLH(
        XMVectorSet(0, 0, -5, 1), XMVectorSet(0, 0, 1, 0), XMVectorSet(0, 1, 0, 0));
    const XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1.f, 0.1f, 100.f);

    CameraCB cb{};
    cb.position = XMVectorSet(0, 0, -5, 1);
    state.FillCameraCB(cb, view, proj);

    const XMMATRIX expectedPrev = XMMatrixMultiply(view, proj);
    const XMMATRIX boundPrev = XMMatrixTranspose(cb.prevViewProjectionMatrix);
    EXPECT_TRUE(MatrixNearEqual(boundPrev, expectedPrev));
    EXPECT_FALSE(state.historyReset);
}

TEST(TemporalCameraStateTests, SecondFrame_PrevIsPreviousUnjitteredVP)
{
    TemporalCameraState state;
    state.enabled = true;

    const XMMATRIX view0 = XMMatrixLookToLH(
        XMVectorSet(0, 0, -5, 1), XMVectorSet(0, 0, 1, 0), XMVectorSet(0, 1, 0, 0));
    const XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1.f, 0.1f, 100.f);
    const XMMATRIX vp0 = XMMatrixMultiply(view0, proj);

    state.BeginFrame(64, 64);
    CameraCB cb0{};
    state.FillCameraCB(cb0, view0, proj);

    const XMMATRIX view1 = XMMatrixLookToLH(
        XMVectorSet(1, 0, -5, 1), XMVectorSet(0, 0, 1, 0), XMVectorSet(0, 1, 0, 0));
    state.BeginFrame(64, 64);
    CameraCB cb1{};
    state.FillCameraCB(cb1, view1, proj);

    const XMMATRIX boundPrev = XMMatrixTranspose(cb1.prevViewProjectionMatrix);
    EXPECT_TRUE(MatrixNearEqual(boundPrev, vp0));
}

TEST(TemporalCameraStateTests, Disabled_ZeroJitterAndUnchangedProjection)
{
    TemporalCameraState state;
    state.enabled = false;
    state.BeginFrame(128, 128);
    EXPECT_FLOAT_EQ(state.currentJitter.x, 0.f);
    EXPECT_FLOAT_EQ(state.currentJitter.y, 0.f);

    const XMMATRIX view = XMMatrixIdentity();
    const XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1.f, 0.1f, 100.f);
    CameraCB cb{};
    state.FillCameraCB(cb, view, proj);

    EXPECT_FLOAT_EQ(cb.jitter.x, 0.f);
    EXPECT_FLOAT_EQ(cb.jitter.y, 0.f);
    EXPECT_TRUE(MatrixNearEqual(XMMatrixTranspose(cb.projectionMatrix), proj));
    EXPECT_TRUE(MatrixNearEqual(XMMatrixTranspose(cb.projectionMatrixUnjittered), proj));
}
