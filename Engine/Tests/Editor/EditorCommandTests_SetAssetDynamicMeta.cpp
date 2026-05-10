#include "EditorCoreFixture.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommandRegistry.h"
#include "Editor/Commands/EditorCommand_SetAssetDynamicMeta.h"

#include "Runtime/Assets/DPrimaryAsset.h"

#include <nlohmann/json.hpp>

#include <vector>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

namespace
{
void SeedDynamicMeta(DPrimaryAsset* asset, nlohmann::json meta)
{
    asset->SetDynamicMeta(std::move(meta));
}
} // namespace

class EditorCommandTests_SetAssetDynamicMeta : public EditorCoreFixture
{
protected:
    void SetUp() override
    {
        EditorCoreFixture::SetUp();
        m_assetId = GetActiveSceneAssetId();
        ASSERT_FALSE(m_assetId.IsNull());
        m_asset = m_core->GetAssetDatabase()->GetLoadedAsset(m_assetId);
        ASSERT_NE(m_asset, nullptr);
        SeedDynamicMeta(m_asset, nlohmann::json{
            {"desc", "initial"},
            {"tags", {"a", "b"}}
        });
        m_core->GetAssetDatabase()->RefreshAssetMetaCache(m_assetId);
    }

    AssetId        m_assetId;
    DPrimaryAsset* m_asset = nullptr;
};

TEST_F(EditorCommandTests_SetAssetDynamicMeta, Execute_SetsDescField)
{
    EditorCommandContext ctx{ *m_core };
    auto cmd = std::make_unique<EditorCommand_SetAssetDynamicMeta>(
        m_assetId, "/dynamic/desc", nlohmann::json("hello"));
    ASSERT_TRUE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));

    EXPECT_EQ(m_asset->GetDynamicMeta()["desc"], "hello");
    EXPECT_EQ(m_core->GetAssetDatabase()->GetAssetMeta(m_assetId)["dynamic"]["desc"], "hello");
    EXPECT_TRUE(m_asset->IsDirty());
}

TEST_F(EditorCommandTests_SetAssetDynamicMeta, Execute_ReplacesTagsArray)
{
    EditorCommandContext ctx{ *m_core };
    auto cmd = std::make_unique<EditorCommand_SetAssetDynamicMeta>(
        m_assetId, "/dynamic/tags", nlohmann::json::array({"x", "y", "z"}));
    ASSERT_TRUE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));

    const auto& tags = m_asset->GetDynamicMeta()["tags"];
    ASSERT_EQ(tags.size(), 3u);
    EXPECT_EQ(tags[0], "x");
    EXPECT_EQ(tags[2], "z");
}

TEST_F(EditorCommandTests_SetAssetDynamicMeta, Execute_AddsExtraNestedKey)
{
    EditorCommandContext ctx{ *m_core };
    auto cmd = std::make_unique<EditorCommand_SetAssetDynamicMeta>(
        m_assetId, "/dynamic/custom/k", nlohmann::json("v"));
    ASSERT_TRUE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));

    EXPECT_EQ(m_asset->GetDynamicMeta()["custom"]["k"], "v");
    EXPECT_EQ(m_core->GetAssetDatabase()->GetAssetMeta(m_assetId)["dynamic"]["custom"]["k"], "v");
}

TEST_F(EditorCommandTests_SetAssetDynamicMeta, Execute_ReplacesWholeDynamicBlob_EmptyPath)
{
    EditorCommandContext ctx{ *m_core };
    nlohmann::json blob{
        {"desc", "whole"},
        {"tags", {"only"}},
        {"extra", 42}
    };
    auto cmd = std::make_unique<EditorCommand_SetAssetDynamicMeta>(
        m_assetId, "", blob);
    ASSERT_TRUE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));

    EXPECT_EQ(m_asset->GetDynamicMeta()["desc"], "whole");
    EXPECT_EQ(m_asset->GetDynamicMeta()["extra"], 42);
}

TEST_F(EditorCommandTests_SetAssetDynamicMeta, Execute_ReplacesWholeDynamicBlob_DynamicPath)
{
    EditorCommandContext ctx{ *m_core };
    nlohmann::json blob{
        {"desc", "via-dynamic"},
        {"tags", nlohmann::json::array()}
    };
    auto cmd = std::make_unique<EditorCommand_SetAssetDynamicMeta>(
        m_assetId, "/dynamic", blob);
    ASSERT_TRUE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));

    EXPECT_EQ(m_asset->GetDynamicMeta()["desc"], "via-dynamic");
    EXPECT_TRUE(m_asset->GetDynamicMeta()["tags"].is_array());
    EXPECT_EQ(m_asset->GetDynamicMeta()["tags"].size(), 0u);
}

TEST_F(EditorCommandTests_SetAssetDynamicMeta, Undo_RevertsExistingValue)
{
    EditorCommandContext ctx{ *m_core };
    auto cmd = std::make_unique<EditorCommand_SetAssetDynamicMeta>(
        m_assetId, "/dynamic/desc", nlohmann::json("changed"));
    ASSERT_TRUE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));
    ASSERT_EQ(m_asset->GetDynamicMeta()["desc"], "changed");

    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));
    EXPECT_EQ(m_asset->GetDynamicMeta()["desc"], "initial");
    EXPECT_EQ(m_core->GetAssetDatabase()->GetAssetMeta(m_assetId)["dynamic"]["desc"], "initial");
}

TEST_F(EditorCommandTests_SetAssetDynamicMeta, Undo_RemovesAddedKey)
{
    EditorCommandContext ctx{ *m_core };
    auto cmd = std::make_unique<EditorCommand_SetAssetDynamicMeta>(
        m_assetId, "/dynamic/extra", nlohmann::json("foo"));
    ASSERT_TRUE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));
    ASSERT_TRUE(m_asset->GetDynamicMeta().contains("extra"));

    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));
    EXPECT_FALSE(m_asset->GetDynamicMeta().contains("extra"));
    EXPECT_FALSE(m_core->GetAssetDatabase()->GetAssetMeta(m_assetId)["dynamic"].contains("extra"));
}

TEST_F(EditorCommandTests_SetAssetDynamicMeta, Redo_ReappliesValue)
{
    EditorCommandContext ctx{ *m_core };
    auto cmd = std::make_unique<EditorCommand_SetAssetDynamicMeta>(
        m_assetId, "/dynamic/desc", nlohmann::json("redo-me"));
    ASSERT_TRUE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));
    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));
    ASSERT_EQ(m_asset->GetDynamicMeta()["desc"], "initial");

    ASSERT_TRUE(m_core->GetCommandManager().Redo(ctx));
    EXPECT_EQ(m_asset->GetDynamicMeta()["desc"], "redo-me");
    EXPECT_EQ(m_core->GetAssetDatabase()->GetAssetMeta(m_assetId)["dynamic"]["desc"], "redo-me");
}

TEST_F(EditorCommandTests_SetAssetDynamicMeta, Reject_StaticPath)
{
    EditorCommandContext ctx{ *m_core };
    const size_t depthBefore = m_core->GetCommandManager().GetUndoStackDepth();
    auto cmd = std::make_unique<EditorCommand_SetAssetDynamicMeta>(
        m_assetId, "/static/foo", nlohmann::json("x"));
    EXPECT_FALSE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));

    EXPECT_EQ(m_core->GetCommandManager().GetUndoStackDepth(), depthBefore);
    EXPECT_EQ(m_asset->GetDynamicMeta()["desc"], "initial");
}

TEST_F(EditorCommandTests_SetAssetDynamicMeta, Reject_RemovesDescField)
{
    EditorCommandContext ctx{ *m_core };
    const size_t depthBefore = m_core->GetCommandManager().GetUndoStackDepth();
    auto cmd = std::make_unique<EditorCommand_SetAssetDynamicMeta>(
        m_assetId, "/dynamic/desc", nlohmann::json(123));
    EXPECT_FALSE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));

    EXPECT_EQ(m_core->GetCommandManager().GetUndoStackDepth(), depthBefore);
    EXPECT_EQ(m_asset->GetDynamicMeta()["desc"], "initial");
}

TEST_F(EditorCommandTests_SetAssetDynamicMeta, Reject_TagsNonArray)
{
    EditorCommandContext ctx{ *m_core };
    auto cmd = std::make_unique<EditorCommand_SetAssetDynamicMeta>(
        m_assetId, "/dynamic/tags", nlohmann::json("nope"));
    EXPECT_FALSE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));
    ASSERT_TRUE(m_asset->GetDynamicMeta()["tags"].is_array());
    EXPECT_EQ(m_asset->GetDynamicMeta()["tags"].size(), 2u);
}

TEST_F(EditorCommandTests_SetAssetDynamicMeta, Reject_TagsNonStringElement)
{
    EditorCommandContext ctx{ *m_core };
    auto cmd = std::make_unique<EditorCommand_SetAssetDynamicMeta>(
        m_assetId, "/dynamic/tags/0", nlohmann::json(42));
    EXPECT_FALSE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));
    EXPECT_EQ(m_asset->GetDynamicMeta()["tags"][0], "a");
}

TEST_F(EditorCommandTests_SetAssetDynamicMeta, Reject_NonDynamicTopLevel)
{
    EditorCommandContext ctx{ *m_core };
    auto cmd = std::make_unique<EditorCommand_SetAssetDynamicMeta>(
        m_assetId, "/random", nlohmann::json("x"));
    EXPECT_FALSE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));
}

TEST_F(EditorCommandTests_SetAssetDynamicMeta, Reject_UnknownAsset)
{
    EditorCommandContext ctx{ *m_core };
    auto cmd = std::make_unique<EditorCommand_SetAssetDynamicMeta>(
        AssetId::Generate(), "/dynamic/desc", nlohmann::json("x"));
    EXPECT_FALSE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));
}

TEST_F(EditorCommandTests_SetAssetDynamicMeta, JsonRoundTrip)
{
    EditorCommandContext ctx{ *m_core };
    auto srcCmd = std::make_unique<EditorCommand_SetAssetDynamicMeta>(
        m_assetId, "/dynamic/desc", nlohmann::json("after"));
    EditorCommand_SetAssetDynamicMeta* srcPtr = srcCmd.get();
    ASSERT_TRUE(m_core->GetCommandManager().Execute(std::move(srcCmd), ctx));

    nlohmann::json out;
    srcPtr->Serialize(out);
    EXPECT_EQ(out["assetId"], m_assetId.ToString());
    EXPECT_EQ(out["jsonPointer"], "/dynamic/desc");
    EXPECT_EQ(out["valueAfter"], "after");
    EXPECT_EQ(out["valueBefore"], "initial");
    EXPECT_TRUE(out["hadValueBefore"].get<bool>());
    EXPECT_TRUE(out["snapshotTaken"].get<bool>());

    EditorCommand_SetAssetDynamicMeta dst;
    dst.Deserialize(out);

    nlohmann::json out2;
    dst.Serialize(out2);
    EXPECT_EQ(out, out2);
}

TEST_F(EditorCommandTests_SetAssetDynamicMeta, FactoryRegistered)
{
    auto created = EditorCommandRegistry::Get().Create(
        EditorCommand_SetAssetDynamicMeta::StaticTypeName());
    ASSERT_NE(created, nullptr);
    EXPECT_EQ(created->GetTypeName(), EditorCommand_SetAssetDynamicMeta::StaticTypeName());
}

TEST_F(EditorCommandTests_SetAssetDynamicMeta, SerializedReplay_ViaQueue)
{
    nlohmann::json data;
    data["assetId"]     = m_assetId.ToString();
    data["jsonPointer"] = "/dynamic/desc";
    data["valueAfter"]  = "from-queue";
    nlohmann::json env;
    env["type"] = std::string(EditorCommand_SetAssetDynamicMeta::StaticTypeName());
    env["data"] = data;

    m_core->EnqueueSerializedCommand(env.dump());
    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);

    ASSERT_EQ(responses.size(), 1u);
    auto resp = nlohmann::json::parse(responses[0]);
    EXPECT_TRUE(resp.value("ok", false));

    EXPECT_EQ(m_asset->GetDynamicMeta()["desc"], "from-queue");
    EXPECT_EQ(m_core->GetAssetDatabase()->GetAssetMeta(m_assetId)["dynamic"]["desc"], "from-queue");
}

TEST_F(EditorCommandTests_SetAssetDynamicMeta, CacheConsistencyAcrossUndoRedo)
{
    EditorCommandContext ctx{ *m_core };

    auto cacheDynamic = [&] {
        return m_core->GetAssetDatabase()->GetAssetMeta(m_assetId)["dynamic"];
    };

    EXPECT_EQ(cacheDynamic(), m_asset->GetDynamicMeta());

    auto cmd = std::make_unique<EditorCommand_SetAssetDynamicMeta>(
        m_assetId, "/dynamic/tags", nlohmann::json::array({"k", "v"}));
    ASSERT_TRUE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));
    EXPECT_EQ(cacheDynamic(), m_asset->GetDynamicMeta());

    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));
    EXPECT_EQ(cacheDynamic(), m_asset->GetDynamicMeta());

    ASSERT_TRUE(m_core->GetCommandManager().Redo(ctx));
    EXPECT_EQ(cacheDynamic(), m_asset->GetDynamicMeta());
}
