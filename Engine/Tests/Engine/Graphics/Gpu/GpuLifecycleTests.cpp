#include "Shared/GpuD3D12Validation.h"
#include "Shared/GpuSceneBuilder.h"
#include "Shared/GpuTestAssetFixture.h"

#include "Runtime/Graphics/DefaultTextures.h"

#include <array>
#include <gtest/gtest.h>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class GpuLifecycleTests : public GpuTestAssetFixture
{
};

TEST_F(GpuLifecycleTests, ResizeStress_MultipleDimensionsIncludingReturnTo64)
{
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NE(builder.AddSphereMesh(), nullptr);
    ASSERT_NE(builder.AddCamera(), nullptr);
    ASSERT_NE(builder.AddDirectionalLightWithShadows(), nullptr);
    builder.InitGpuResources();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    const std::array<std::pair<uint32_t, uint32_t>, 5> sizes = {
        {{128, 128}, {96, 96}, {64, 64}, {32, 32}, {64, 64}}};

    DXRenderManager& renderManager = GetRenderManager();
    for (const auto& [width, height] : sizes)
    {
        renderManager.Resize(width, height);
        EXPECT_EQ(renderManager.GetWidth(), width);
        EXPECT_EQ(renderManager.GetHeight(), height);
        EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

        RenderSceneFrame();
        EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
    }
}

TEST_F(GpuLifecycleTests, FullCycle_RenderOnDestroyRecreate_Succeeds)
{
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NE(builder.AddSphereMesh(), nullptr);
    ASSERT_NE(builder.AddCamera(), nullptr);
    ASSERT_NE(builder.AddDirectionalLightWithShadows(), nullptr);
    builder.InitGpuResources();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    RenderSceneFrame();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    ReinitializeRenderPipeline();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    GpuSceneBuilder rebuilt(GetEngine(), GetAssetDatabase());
    ASSERT_NE(rebuilt.AddSphereMesh(), nullptr);
    ASSERT_NE(rebuilt.AddCamera(), nullptr);
    ASSERT_NE(rebuilt.AddDirectionalLightWithShadows(), nullptr);
    rebuilt.InitGpuResources();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    RenderSceneFrame();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuLifecycleTests, TeardownAudit_NoValidationErrorsOrLiveObjects)
{
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NE(builder.AddSphereMesh(), nullptr);
    ASSERT_NE(builder.AddCamera(), nullptr);
    ASSERT_NE(builder.AddDirectionalLightWithShadows(), nullptr);
    builder.InitGpuResources();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    RenderSceneFrame();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    GetEngine().Cleanup();
    AssertGpuTeardownClean(*GetDevice());
}
