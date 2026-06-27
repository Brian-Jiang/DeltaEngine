#include "Shared/GpuTestAssetFixture.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class GpuFrameSmokeTests : public GpuTestAssetFixture
{
};

// Disabled: this test asserts specific placeholder imported assets exist by name
// (DefaultScene, Sphere/SphereMesh). Those assets are placeholders, not permanent
// engine assets, so the expectations are not stable.
#if 0
TEST_F(GpuFrameSmokeTests, ScanImportedAssets_FindsExpectedEntries)
{
    const AssetId defaultSceneId = FindImportedAssetId("DefaultScene");
    EXPECT_FALSE(defaultSceneId.IsNull());

    const AssetId sphereMeshId = FindImportedAssetId("Sphere/SphereMesh");
    EXPECT_FALSE(sphereMeshId.IsNull());

    EXPECT_GT(GetAssetDatabase().GetAllAssets().size(), 2u);
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}
#endif

TEST_F(GpuFrameSmokeTests, EmptyWorld_RendersAndResizes_WithCleanValidation)
{
    DXRenderManager& renderManager = GetRenderManager();
    EXPECT_EQ(renderManager.GetWidth(), 64u);
    EXPECT_EQ(renderManager.GetHeight(), 64u);

    RenderSceneFrame();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    renderManager.Resize(128, 128);
    EXPECT_EQ(renderManager.GetWidth(), 128u);
    EXPECT_EQ(renderManager.GetHeight(), 128u);
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    RenderSceneFrame();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}
