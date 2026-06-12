#include "Shared/GpuSceneBuilder.h"
#include "Shared/GpuTestAssetFixture.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class GpuMeshAndShadowTests : public GpuTestAssetFixture
{
};

TEST_F(GpuMeshAndShadowTests, SphereMesh_InitWorldRenderers_UploadsWithoutValidationErrors)
{
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NE(builder.AddSphereMesh(), nullptr);

    builder.InitGpuResources();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuMeshAndShadowTests, SphereAndCamera_RendersFrame_WithCleanValidation)
{
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NE(builder.AddSphereMesh(), nullptr);
    ASSERT_NE(builder.AddCamera(), nullptr);

    builder.InitGpuResources();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    RenderSceneFrame();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuMeshAndShadowTests, DirectionalLight_RendersShadowFrame_WithCleanValidation)
{
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NE(builder.AddSphereMesh(), nullptr);
    ASSERT_NE(builder.AddCamera(), nullptr);
    ASSERT_NE(builder.AddDirectionalLightWithShadows(), nullptr);

    builder.InitGpuResources();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    RenderSceneFrame();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuMeshAndShadowTests, PointLight_RendersShadowFrame_WithCleanValidation)
{
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NE(builder.AddSphereMesh(), nullptr);
    ASSERT_NE(builder.AddCamera(), nullptr);
    ASSERT_NE(builder.AddPointLightWithShadows(), nullptr);

    builder.InitGpuResources();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    RenderSceneFrame();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuMeshAndShadowTests, SpotLight_RendersShadowFrame_WithCleanValidation)
{
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NE(builder.AddSphereMesh(), nullptr);
    ASSERT_NE(builder.AddCamera(), nullptr);
    ASSERT_NE(builder.AddSpotLightWithShadows(), nullptr);

    builder.InitGpuResources();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    RenderSceneFrame();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuMeshAndShadowTests, AllLightTypes_RendersCombinedShadowFrame_WithCleanValidation)
{
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NE(builder.AddSphereMesh(), nullptr);
    ASSERT_NE(builder.AddCamera(), nullptr);
    ASSERT_NE(builder.AddDirectionalLightWithShadows(), nullptr);
    ASSERT_NE(builder.AddPointLightWithShadows(), nullptr);
    ASSERT_NE(builder.AddSpotLightWithShadows(), nullptr);

    builder.InitGpuResources();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    RenderSceneFrame();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}
