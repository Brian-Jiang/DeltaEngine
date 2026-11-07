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

TEST_F(GpuTemporalResetTests, Resize_RecreatesHistoryTexturesAndSetsReset)
{
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NE(builder.AddCamera(), nullptr);
    builder.InitGpuResources();

    DXRenderManager& renderManager = GetRenderManager();
    renderManager.SetTemporalEnabled(true);
    RenderSceneFrames(2);

    const TemporalHistory& history = renderManager.GetTemporalHistory();
    ASSERT_TRUE(history.HasTextures());
    const auto oldRead = history.GetReadTexture();
    const auto oldWrite = history.GetWriteTexture();
    ASSERT_FALSE(renderManager.GetTemporalCameraState().historyReset);

    const UINT newWidth = renderManager.GetWidth() + 64;
    const UINT newHeight = renderManager.GetHeight() + 32;
    renderManager.Resize(newWidth, newHeight);

    ASSERT_TRUE(history.HasTextures());
    EXPECT_NE(history.GetReadTexture(), oldRead);
    EXPECT_NE(history.GetReadTexture(), oldWrite);
    EXPECT_NE(history.GetWriteTexture(), oldRead);
    EXPECT_NE(history.GetWriteTexture(), oldWrite);
    for (const auto* texture : { history.GetReadTexture().get(), history.GetWriteTexture().get() })
    {
        const D3D12_RESOURCE_DESC desc = texture->GetD3D12ResourceDesc();
        EXPECT_EQ(desc.Width, newWidth);
        EXPECT_EQ(desc.Height, newHeight);
        EXPECT_EQ(desc.Format, kTemporalHistoryFormat);
        EXPECT_EQ(desc.SampleDesc.Count, 1u);
    }
    EXPECT_FALSE(history.IsValid());
    EXPECT_TRUE(renderManager.GetTemporalCameraState().historyReset);

    RenderSceneFrame();
    EXPECT_FALSE(history.IsValid());
}
