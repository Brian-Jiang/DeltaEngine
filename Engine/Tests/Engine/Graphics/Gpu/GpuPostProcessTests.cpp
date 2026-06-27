#include "Shared/GpuReadback.h"
#include "Shared/GpuSceneBuilder.h"
#include "Shared/GpuTestAssetFixture.h"

#include "Runtime/Graphics/PostProcess/PostProcessStack.h"

#include <cmath>
#include <gtest/gtest.h>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class GpuPostProcessTests : public GpuTestAssetFixture
{
};

TEST_F(GpuPostProcessTests, PassthroughAndTonemap_RendersFinalOutput_WithNonClearCenterPixel)
{
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NE(builder.AddSphereMesh(), nullptr);
    Camera* camera = builder.AddCamera();
    ASSERT_NE(camera, nullptr);
    ASSERT_NE(builder.AddDirectionalLightWithShadows(), nullptr);

    PostProcessStack* stack = builder.CreatePassthroughTonemapStack();
    ASSERT_NE(stack, nullptr);
    EXPECT_EQ(stack->GetPassCount(), 2);
    builder.AttachPostProcessStack(camera, stack);

    builder.InitGpuResources();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    RenderSceneFrame();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    DXRenderManager& renderManager = GetRenderManager();
    EXPECT_TRUE(renderManager.HasPostProcessedOutput());

    const std::shared_ptr<DirectX12Texture> finalTexture = renderManager.GetFinalPostProcessTexture();
    ASSERT_NE(finalTexture, nullptr);
    EXPECT_EQ(GetFinalColorTexture(), finalTexture);

    GpuReadbackPixel centerPixel{};
    ASSERT_TRUE(ReadTextureCenterPixel(*GetDevice(), GetDirectQueue(), finalTexture, centerPixel));
    // The ping-pong output texture is cleared to (0,0,0,1) before each pass.
    // Verifying any channel is nonzero confirms the passthrough+tonemap chain
    // actually executed and wrote scene content to the final texture.
    EXPECT_TRUE(
        std::fabs(centerPixel.r) > 0.01f || std::fabs(centerPixel.g) > 0.01f
        || std::fabs(centerPixel.b) > 0.01f);
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuPostProcessTests, BloomTonemap_RendersFinalOutput_WithNonClearCenterPixel)
{
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NE(builder.AddSphereMesh(), nullptr);
    Camera* camera = builder.AddCamera();
    ASSERT_NE(camera, nullptr);
    ASSERT_NE(builder.AddDirectionalLightWithShadows(), nullptr);

    PostProcessStack* stack = builder.CreateBloomTonemapStack();
    ASSERT_NE(stack, nullptr);
    EXPECT_EQ(stack->GetPassCount(), 2);
    builder.AttachPostProcessStack(camera, stack);

    builder.InitGpuResources();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    RenderSceneFrame();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    DXRenderManager& renderManager = GetRenderManager();
    EXPECT_TRUE(renderManager.HasPostProcessedOutput());

    const std::shared_ptr<DirectX12Texture> finalTexture = renderManager.GetFinalPostProcessTexture();
    ASSERT_NE(finalTexture, nullptr);
    EXPECT_EQ(GetFinalColorTexture(), finalTexture);

    GpuReadbackPixel centerPixel{};
    ASSERT_TRUE(ReadTextureCenterPixel(*GetDevice(), GetDirectQueue(), finalTexture, centerPixel));
    EXPECT_TRUE(
        std::fabs(centerPixel.r) > 0.01f || std::fabs(centerPixel.g) > 0.01f
        || std::fabs(centerPixel.b) > 0.01f);
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}
