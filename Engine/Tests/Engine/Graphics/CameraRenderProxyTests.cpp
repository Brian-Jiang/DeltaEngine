#include "Runtime/Graphics/RenderProxy/CameraRenderProxy.h"

#include <DirectXMath.h>
#include <gtest/gtest.h>

using namespace DeltaEngine;
using namespace DirectX;

TEST(CameraRenderProxyTests, CameraRenderProxy_ConstructWithValidParams_StoresThem)
{
    // Arrange / Act
    CameraRenderProxy cam(XM_PIDIV4, 16.0f / 9.0f, 0.1f, 100.0f);

    // Assert
    EXPECT_FLOAT_EQ(cam.GetFov(), XM_PIDIV4);
    EXPECT_FLOAT_EQ(cam.GetAspectRatio(), 16.0f / 9.0f);
    EXPECT_FLOAT_EQ(cam.GetNearPlane(), 0.1f);
    EXPECT_FLOAT_EQ(cam.GetFarPlane(), 100.0f);
}

TEST(CameraRenderProxyTests, CameraRenderProxy_ConstructWithInvalidFov_FallsBackToDefaults)
{
    // Arrange / Act
    CameraRenderProxy cam(0.0f, 1.0f, 0.1f, 100.0f);

    // Assert (defaults from header)
    EXPECT_GT(cam.GetFov(), 0.0f);
    EXPECT_GT(cam.GetNearPlane(), 0.0f);
    EXPECT_GT(cam.GetFarPlane(), cam.GetNearPlane());
}

TEST(CameraRenderProxyTests, CameraRenderProxy_UpdateParametersInvalid_KeepsLastGoodValues)
{
    // Arrange
    CameraRenderProxy cam(XM_PIDIV4, 16.0f / 9.0f, 0.1f, 100.0f);

    // Act: farPlane <= nearPlane is invalid, must be rejected
    cam.UpdateParameters(XM_PIDIV4, 16.0f / 9.0f, 10.0f, 5.0f);

    // Assert: original values preserved
    EXPECT_FLOAT_EQ(cam.GetNearPlane(), 0.1f);
    EXPECT_FLOAT_EQ(cam.GetFarPlane(), 100.0f);
}

TEST(CameraRenderProxyTests, CameraRenderProxy_UpdateAspectRatioInvalid_KeepsLastGood)
{
    // Arrange
    CameraRenderProxy cam(XM_PIDIV4, 16.0f / 9.0f, 0.1f, 100.0f);

    // Act
    cam.UpdateAspectRatio(-1.0f);

    // Assert
    EXPECT_FLOAT_EQ(cam.GetAspectRatio(), 16.0f / 9.0f);
}

TEST(CameraRenderProxyTests, CameraRenderProxy_UpdateAspectRatioValid_Updates)
{
    // Arrange
    CameraRenderProxy cam(XM_PIDIV4, 1.0f, 0.1f, 100.0f);

    // Act
    cam.UpdateAspectRatio(2.0f);

    // Assert
    EXPECT_FLOAT_EQ(cam.GetAspectRatio(), 2.0f);
}

TEST(CameraRenderProxyTests, CameraRenderProxy_PostProcessStackAccessor_RoundTrips)
{
    // Arrange
    CameraRenderProxy cam(XM_PIDIV4, 1.0f, 0.1f, 100.0f);
    EXPECT_EQ(cam.GetPostProcessStack(), nullptr);

    // Act
    cam.SetPostProcessStack(reinterpret_cast<PostProcessStack*>(0x1234));

    // Assert
    EXPECT_EQ(cam.GetPostProcessStack(), reinterpret_cast<PostProcessStack*>(0x1234));

    // Cleanup
    cam.SetPostProcessStack(nullptr);
}

TEST(CameraRenderProxyTests, CameraRenderProxy_UpdateTransform_RecomputesViewMatrix)
{
    // Arrange
    CameraRenderProxy cam(XM_PIDIV4, 1.0f, 0.1f, 100.0f);
    const XMMATRIX before = cam.GetViewMatrix();

    // Act: move camera; view matrix should change
    XMMATRIX world = XMMatrixTranslation(5.0f, 0.0f, 0.0f);
    cam.UpdateTransform(world);

    // Assert
    const XMMATRIX after = cam.GetViewMatrix();
    EXPECT_NE(XMVectorGetX(after.r[3]), XMVectorGetX(before.r[3]));
}
