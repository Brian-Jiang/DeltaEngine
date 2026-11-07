#include "Shared/GpuSceneBuilder.h"
#include "Shared/GpuTestAssetFixture.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class GpuTemporalResetTests : public GpuTestAssetFixture
{
};

TEST_F(GpuTemporalResetTests, InitWorldRenderers_ResetsHistoryAndRestartsHalton)
{
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NE(builder.AddCamera(), nullptr);
    builder.InitGpuResources();

    DXRenderManager& renderManager = GetRenderManager();
    renderManager.SetTemporalEnabled(true);
    EXPECT_TRUE(renderManager.IsTemporalEnabled());

    RenderSceneFrames(2);
    const TemporalCameraState& temporal = renderManager.GetTemporalCameraState();
    ASSERT_FALSE(temporal.historyReset);
    ASSERT_GT(temporal.haltonIndex, 0u);

    renderManager.InitWorldRenderers(*GetEngine().GetWorld());
    GetDevice()->Flush();

    EXPECT_TRUE(temporal.historyReset);
    EXPECT_EQ(temporal.haltonIndex, 0u);
}
