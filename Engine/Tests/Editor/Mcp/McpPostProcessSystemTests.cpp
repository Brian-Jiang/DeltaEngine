#include "Editor/Mcp/McpCoreFixture.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Runtime/Graphics/PostProcess/PA_PostProcessStack.h"
#include "Runtime/Graphics/PostProcess/PostProcessPass.h"
#include "Runtime/Graphics/PostProcess/PostProcessStack.h"
#include "Runtime/Reflection/DClass.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class McpPostProcessSystemTests : public McpCoreFixture
{
protected:
    PA_PostProcessStack* CreateRegisteredStack(const std::string& fileStem)
    {
        auto* asset = PA_PostProcessStack::Create();
        if (!asset)
            return nullptr;
        const auto path = m_tempDir / (fileStem + ".dasset.json");
        m_core->GetAssetDatabase()->CreateAsset(path, asset);
        return asset;
    }
};

TEST_F(McpPostProcessSystemTests, CommandAddPass_AddsPassToStackAsset)
{
    PA_PostProcessStack* asset = CreateRegisteredStack("PpStackAdd");
    ASSERT_NE(asset, nullptr);
    const AssetId assetId = asset->GetAssetId();
    ASSERT_FALSE(assetId.IsNull());
    EXPECT_EQ(asset->GetStack()->GetPassCount(), 0);

    auto dispatchRes = Dispatch("post_process", "AddPass",
        {{"assetId", assetId.ToString()}, {"passClass", "TonemapPass"}});
    ASSERT_TRUE(dispatchRes["ok"].get<bool>());
    const json& drainRes = dispatchRes;
    ASSERT_FALSE(drainRes.value("objectId", "").empty());

    asset = dynamic_cast<PA_PostProcessStack*>(m_core->GetAssetDatabase()->LoadAsset(assetId));
    ASSERT_NE(asset, nullptr);
    ASSERT_NE(asset->GetStack(), nullptr);
    ASSERT_EQ(asset->GetStack()->GetPassCount(), 1);
    PostProcessPass* pass = asset->GetStack()->GetPass(0);
    ASSERT_NE(pass, nullptr);
    ASSERT_NE(pass->GetClass(), nullptr);
    EXPECT_EQ(pass->GetClass()->GetName(), "TonemapPass");
    EXPECT_EQ(pass->GetObjectId().ToString(), drainRes["objectId"].get<std::string>());
}

TEST_F(McpPostProcessSystemTests, CommandAddPass_DuplicateClass_Fails)
{
    PA_PostProcessStack* asset = CreateRegisteredStack("PpStackDup");
    ASSERT_NE(asset, nullptr);
    const AssetId assetId = asset->GetAssetId();
    ASSERT_NE(asset->AddPass("BloomPass"), nullptr);

    auto dispatchRes = Dispatch("post_process", "AddPass",
        {{"assetId", assetId.ToString()}, {"passClass", "BloomPass"}});
    EXPECT_FALSE(dispatchRes["ok"].get<bool>());

    asset = dynamic_cast<PA_PostProcessStack*>(m_core->GetAssetDatabase()->LoadAsset(assetId));
    ASSERT_NE(asset, nullptr);
    EXPECT_EQ(asset->GetStack()->GetPassCount(), 1);
}

TEST_F(McpPostProcessSystemTests, CommandRemovePass_RemovesPassFromStackAsset)
{
    PA_PostProcessStack* asset = CreateRegisteredStack("PpStackRemove");
    ASSERT_NE(asset, nullptr);
    const AssetId assetId = asset->GetAssetId();
    ASSERT_NE(asset->AddPass("VignettePass"), nullptr);
    ASSERT_EQ(asset->GetStack()->GetPassCount(), 1);

    auto dispatchRes = Dispatch("post_process", "RemovePass",
        {{"assetId", assetId.ToString()}, {"passClass", "VignettePass"}});
    EXPECT_TRUE(dispatchRes["ok"].get<bool>());

    asset = dynamic_cast<PA_PostProcessStack*>(m_core->GetAssetDatabase()->LoadAsset(assetId));
    ASSERT_NE(asset, nullptr);
    EXPECT_EQ(asset->GetStack()->GetPassCount(), 0);
}

TEST_F(McpPostProcessSystemTests, CommandAddPass_Undo_RemovesPass)
{
    PA_PostProcessStack* asset = CreateRegisteredStack("PpStackUndo");
    ASSERT_NE(asset, nullptr);
    const AssetId assetId = asset->GetAssetId();

    auto dispatchRes = Dispatch("post_process", "AddPass",
        {{"assetId", assetId.ToString()}, {"passClass", "ColorGradingPass"}});
    ASSERT_TRUE(dispatchRes["ok"].get<bool>());

    asset = dynamic_cast<PA_PostProcessStack*>(m_core->GetAssetDatabase()->LoadAsset(assetId));
    ASSERT_NE(asset, nullptr);
    ASSERT_EQ(asset->GetStack()->GetPassCount(), 1);

    auto undoRes = Dispatch("undo_history", "Undo");
    ASSERT_TRUE(undoRes["ok"].get<bool>());

    asset = dynamic_cast<PA_PostProcessStack*>(m_core->GetAssetDatabase()->LoadAsset(assetId));
    ASSERT_NE(asset, nullptr);
    EXPECT_EQ(asset->GetStack()->GetPassCount(), 0);
}

TEST_F(McpPostProcessSystemTests, CommandRemovePass_Undo_RestoresPass)
{
    PA_PostProcessStack* asset = CreateRegisteredStack("PpStackRemoveUndo");
    ASSERT_NE(asset, nullptr);
    const AssetId assetId = asset->GetAssetId();
    ASSERT_NE(asset->AddPass("PassthroughPass"), nullptr);

    auto dispatchRes = Dispatch("post_process", "RemovePass",
        {{"assetId", assetId.ToString()}, {"passClass", "PassthroughPass"}});
    ASSERT_TRUE(dispatchRes["ok"].get<bool>());

    asset = dynamic_cast<PA_PostProcessStack*>(m_core->GetAssetDatabase()->LoadAsset(assetId));
    ASSERT_NE(asset, nullptr);
    ASSERT_EQ(asset->GetStack()->GetPassCount(), 0);

    auto undoRes = Dispatch("undo_history", "Undo");
    ASSERT_TRUE(undoRes["ok"].get<bool>());

    asset = dynamic_cast<PA_PostProcessStack*>(m_core->GetAssetDatabase()->LoadAsset(assetId));
    ASSERT_NE(asset, nullptr);
    ASSERT_EQ(asset->GetStack()->GetPassCount(), 1);
    PostProcessPass* pass = asset->GetStack()->GetPass(0);
    ASSERT_NE(pass, nullptr);
    ASSERT_NE(pass->GetClass(), nullptr);
    EXPECT_EQ(pass->GetClass()->GetName(), "PassthroughPass");
}
