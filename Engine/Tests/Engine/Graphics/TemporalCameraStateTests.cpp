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
    state.FillCameraCB(cb, view, proj, TemporalCameraSource::SceneCamera);

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
    state.FillCameraCB(cb0, view0, proj, TemporalCameraSource::SceneCamera);

    const XMMATRIX view1 = XMMatrixLookToLH(
        XMVectorSet(1, 0, -5, 1), XMVectorSet(0, 0, 1, 0), XMVectorSet(0, 1, 0, 0));
    state.BeginFrame(64, 64);
    CameraCB cb1{};
    state.FillCameraCB(cb1, view1, proj, TemporalCameraSource::SceneCamera);

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
    state.FillCameraCB(cb, view, proj, TemporalCameraSource::SceneCamera);

    EXPECT_FLOAT_EQ(cb.jitter.x, 0.f);
    EXPECT_FLOAT_EQ(cb.jitter.y, 0.f);
    EXPECT_TRUE(MatrixNearEqual(XMMatrixTranspose(cb.projectionMatrix), proj));
    EXPECT_TRUE(MatrixNearEqual(XMMatrixTranspose(cb.projectionMatrixUnjittered), proj));
}

namespace
{
constexpr uint32_t kSize = 64;

XMMATRIX TestProjection()
{
    return XMMatrixPerspectiveFovLH(XM_PIDIV4, 1.f, 0.1f, 100.f);
}

XMMATRIX ViewAt(float x, float yawRadians = 0.f)
{
    const XMVECTOR forward = XMVectorSet(std::sin(yawRadians), 0, std::cos(yawRadians), 0);
    return XMMatrixLookToLH(XMVectorSet(x, 0, -5, 1), forward, XMVectorSet(0, 1, 0, 0));
}

CameraCB RunFrame(TemporalCameraState& state, FXMMATRIX view, TemporalCameraSource source)
{
    state.BeginFrame(kSize, kSize);
    CameraCB cb{};
    state.FillCameraCB(cb, view, TestProjection(), source);
    return cb;
}

void ExpectHistoryDiscarded(const TemporalCameraState& state, const CameraCB& cb, FXMMATRIX view)
{
    EXPECT_TRUE(state.historyResetThisFrame);
    EXPECT_EQ(state.haltonIndex, 0u);
    EXPECT_TRUE(MatrixNearEqual(XMMatrixTranspose(cb.prevViewProjectionMatrix),
        XMMatrixMultiply(view, TestProjection())));
}
} // namespace

TEST(TemporalCameraStateTests, SetEnabled_On_RequestsResetAndRestartsHalton)
{
    TemporalCameraState state;
    RunFrame(state, ViewAt(0), TemporalCameraSource::SceneCamera);
    state.haltonIndex = 5;

    state.SetEnabled(true);

    EXPECT_TRUE(state.enabled);
    EXPECT_TRUE(state.historyReset);
    EXPECT_EQ(state.haltonIndex, 0u);
    state.BeginFrame(kSize, kSize);
    const XMFLOAT2 expected = HaltonJitterNdc(0, kSize, kSize);
    EXPECT_FLOAT_EQ(state.currentJitter.x, expected.x);
    EXPECT_FLOAT_EQ(state.currentJitter.y, expected.y);
}

TEST(TemporalCameraStateTests, SetEnabled_Off_RequestsReset)
{
    TemporalCameraState state;
    state.SetEnabled(true);
    RunFrame(state, ViewAt(0), TemporalCameraSource::SceneCamera);
    RunFrame(state, ViewAt(0), TemporalCameraSource::SceneCamera);
    ASSERT_FALSE(state.historyReset);

    state.SetEnabled(false);

    EXPECT_FALSE(state.enabled);
    EXPECT_TRUE(state.historyReset);
    EXPECT_EQ(state.haltonIndex, 0u);
}

TEST(TemporalCameraStateTests, SetEnabled_SameValue_NoReset)
{
    TemporalCameraState state;
    state.SetEnabled(true);
    RunFrame(state, ViewAt(0), TemporalCameraSource::SceneCamera);
    ASSERT_FALSE(state.historyReset);
    const uint32_t index = state.haltonIndex;

    state.SetEnabled(true);

    EXPECT_FALSE(state.historyReset);
    EXPECT_EQ(state.haltonIndex, index);
}

TEST(TemporalCameraStateTests, RequestHistoryReset_RestartsHaltonAtSampleZero)
{
    TemporalCameraState state;
    state.SetEnabled(true);
    for (int i = 0; i < 3; ++i)
        RunFrame(state, ViewAt(0), TemporalCameraSource::SceneCamera);
    ASSERT_EQ(state.haltonIndex, 3u);

    state.RequestHistoryReset();
    const CameraCB cb = RunFrame(state, ViewAt(0), TemporalCameraSource::SceneCamera);

    const XMFLOAT2 expected = HaltonJitterNdc(0, kSize, kSize);
    EXPECT_FLOAT_EQ(cb.jitter.x, expected.x);
    EXPECT_FLOAT_EQ(cb.jitter.y, expected.y);
    EXPECT_TRUE(state.historyResetThisFrame);
}

TEST(TemporalCameraStateTests, SourceSwitch_ActiveToScene_ResetsHistory)
{
    TemporalCameraState state;
    state.SetEnabled(true);
    RunFrame(state, ViewAt(0), TemporalCameraSource::ActiveRenderCamera);
    RunFrame(state, ViewAt(0), TemporalCameraSource::ActiveRenderCamera);
    ASSERT_FALSE(state.historyResetThisFrame);

    const XMMATRIX view = ViewAt(0.5f);
    const CameraCB cb = RunFrame(state, view, TemporalCameraSource::SceneCamera);

    ExpectHistoryDiscarded(state, cb, view);
    EXPECT_EQ(state.prevSource, TemporalCameraSource::SceneCamera);
    EXPECT_FALSE(state.historyReset);
}

TEST(TemporalCameraStateTests, SameSource_SmallMove_KeepsHistory)
{
    TemporalCameraState state;
    state.SetEnabled(true);
    RunFrame(state, ViewAt(0), TemporalCameraSource::SceneCamera);

    const CameraCB cb = RunFrame(state, ViewAt(1.f, XMConvertToRadians(10.f)), TemporalCameraSource::SceneCamera);

    EXPECT_FALSE(state.historyResetThisFrame);
    EXPECT_EQ(state.haltonIndex, 2u);
    EXPECT_TRUE(MatrixNearEqual(XMMatrixTranspose(cb.prevViewProjectionMatrix),
        XMMatrixMultiply(ViewAt(0), TestProjection())));
}

TEST(TemporalCameraStateTests, CameraJump_BeyondDistance_ResetsHistory)
{
    TemporalCameraState state;
    state.SetEnabled(true);
    RunFrame(state, ViewAt(0), TemporalCameraSource::SceneCamera);

    const XMMATRIX view = ViewAt(state.cutDistanceThreshold + 1.f);
    const CameraCB cb = RunFrame(state, view, TemporalCameraSource::SceneCamera);

    ExpectHistoryDiscarded(state, cb, view);
}

TEST(TemporalCameraStateTests, CameraJump_BeyondAngle_ResetsHistory)
{
    TemporalCameraState state;
    state.SetEnabled(true);
    RunFrame(state, ViewAt(0), TemporalCameraSource::SceneCamera);

    const XMMATRIX view = ViewAt(0, state.cutAngleThresholdRadians + XMConvertToRadians(5.f));
    const CameraCB cb = RunFrame(state, view, TemporalCameraSource::SceneCamera);

    ExpectHistoryDiscarded(state, cb, view);
}

TEST(TemporalCameraStateTests, TemporalTextureMipBias_NativeAndUpscaled)
{
    EXPECT_FLOAT_EQ(TemporalTextureMipBias(1.f), kNativeTemporalMipBias);
    EXPECT_FLOAT_EQ(TemporalTextureMipBias(2.f), -1.f);
}

TEST(TemporalCameraStateTests, FillCameraCB_WritesMipBiasOnlyWhenEnabled)
{
    const XMMATRIX view = XMMatrixIdentity();
    const XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1.f, 0.1f, 100.f);

    TemporalCameraState enabledState;
    enabledState.enabled = true;
    enabledState.BeginFrame(64, 64);
    CameraCB enabledCb{};
    enabledState.FillCameraCB(enabledCb, view, proj, TemporalCameraSource::SceneCamera);
    EXPECT_FLOAT_EQ(enabledCb.mipBias, kNativeTemporalMipBias);

    TemporalCameraState disabledState;
    disabledState.BeginFrame(64, 64);
    CameraCB disabledCb{};
    disabledState.FillCameraCB(disabledCb, view, proj, TemporalCameraSource::SceneCamera);
    EXPECT_FLOAT_EQ(disabledCb.mipBias, 0.f);
}
