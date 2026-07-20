#include "Editor/Mcp/McpCoreFixture.h"

#include "Editor/Assets/EditorAssetDatabase.h"

#include "Runtime/Assets/DPrimaryAsset.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class McpAssetsMetaTests : public McpCoreFixture
{
protected:
    void SetUp() override
    {
        McpCoreFixture::SetUp();
        m_sceneId = GetActiveSceneAssetId();
        ASSERT_FALSE(m_sceneId.IsNull());
        m_sceneAsset = m_core->GetAssetDatabase()->GetLoadedAsset(m_sceneId);
        ASSERT_NE(m_sceneAsset, nullptr);

        m_sceneAsset->SetDynamicMeta(json{
            {"desc", "initial"},
            {"tags", {"a", "b"}}
        });
        m_core->GetAssetDatabase()->RefreshAssetMetaCache(m_sceneId);
    }

    AssetId        m_sceneId;
    DPrimaryAsset* m_sceneAsset = nullptr;
};

TEST_F(McpAssetsMetaTests, GetAssetMetadata_ReturnsCachedBlob)
{
    auto res = Dispatch("assets", "get_asset_metadata", {{"asset_id", m_sceneId.ToString()}});
    ASSERT_TRUE(res["ok"].get<bool>());
    EXPECT_EQ(res["asset_id"].get<std::string>(), m_sceneId.ToString());
    EXPECT_EQ(res["class"].get<std::string>(), "PA_DScene");
    ASSERT_TRUE(res["meta"].is_object());
    ASSERT_TRUE(res["meta"]["dynamic"].is_object());
    EXPECT_EQ(res["meta"]["dynamic"]["desc"], "initial");
    EXPECT_TRUE(res["meta"]["static"].is_object());
}

TEST_F(McpAssetsMetaTests, GetAssetMetadata_HasStaticMetaSchemaFlagIsFalseForScene)
{
    auto res = Dispatch("assets", "get_asset_metadata", {{"asset_id", m_sceneId.ToString()}});
    ASSERT_TRUE(res["ok"].get<bool>());
    EXPECT_FALSE(res["has_static_meta_schema"].get<bool>());
}

TEST_F(McpAssetsMetaTests, GetAssetMetadata_InvalidId_ReturnsError)
{
    auto res = Dispatch("assets", "get_asset_metadata",
                        {{"asset_id", "00000000-0000-0000-0000-000000000000"}});
    EXPECT_FALSE(res["ok"].get<bool>());
}

TEST_F(McpAssetsMetaTests, GetAssetsMetadata_BulkReturnsAllAssets)
{
    auto res = Dispatch("assets", "get_assets_metadata");
    ASSERT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res["items"].is_array());

    bool foundScene = false;
    for (const auto& item : res["items"])
    {
        if (item["asset_id"].get<std::string>() != m_sceneId.ToString())
            continue;
        foundScene = true;
        EXPECT_EQ(item["class"], "PA_DScene");
        EXPECT_EQ(item["type"], "scene");
        const auto& cached = m_core->GetAssetDatabase()->GetAssetMeta(m_sceneId);
        EXPECT_EQ(item["meta"], cached);
        EXPECT_FALSE(item["has_static_meta_schema"].get<bool>());
    }
    EXPECT_TRUE(foundScene);
}

TEST_F(McpAssetsMetaTests, GetAssetsMetadata_FilterByAssetIds)
{
    json params;
    params["asset_ids"] = json::array({m_sceneId.ToString()});
    auto res = Dispatch("assets", "get_assets_metadata", params);
    ASSERT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res["items"].is_array());
    ASSERT_EQ(res["items"].size(), 1u);
    EXPECT_EQ(res["items"][0]["asset_id"].get<std::string>(), m_sceneId.ToString());
}

TEST_F(McpAssetsMetaTests, GetAssetsMetadata_FilterByType)
{
    auto res = Dispatch("assets", "get_assets_metadata", {{"type_filter", "scene"}});
    ASSERT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res["items"].is_array());
    for (const auto& item : res["items"])
        EXPECT_EQ(item["type"], "scene");
}

TEST_F(McpAssetsMetaTests, SetAssetDynamicMetadata_DispatchesThroughCommandQueue)
{
    json params;
    params["asset_id"]  = m_sceneId.ToString();
    params["json_path"] = "/dynamic/desc";
    params["new_value"] = "from-mcp";
    auto res = Dispatch("assets", "set_asset_dynamic_metadata", params);
    ASSERT_TRUE(res["ok"].get<bool>());

    EXPECT_EQ(m_sceneAsset->GetDynamicMeta()["desc"], "from-mcp");
    EXPECT_EQ(m_core->GetAssetDatabase()->GetAssetMeta(m_sceneId)["dynamic"]["desc"], "from-mcp");
}

TEST_F(McpAssetsMetaTests, SetAssetDynamicMetadata_PersistsToDiskAfterSave)
{
    json params;
    params["asset_id"]  = m_sceneId.ToString();
    params["json_path"] = "/dynamic/desc";
    params["new_value"] = "persisted";
    Dispatch("assets", "set_asset_dynamic_metadata", params);


    m_core->GetAssetDatabase()->SaveAsset(m_sceneId);

    const std::filesystem::path path = m_core->GetAssetDatabase()->GetAssetPath(m_sceneId);
    ASSERT_TRUE(std::filesystem::exists(path));

    std::ifstream in(path);
    ASSERT_TRUE(in.is_open());
    json root = json::parse(in);
    ASSERT_TRUE(root.contains("meta"));
    ASSERT_TRUE(root["meta"]["dynamic"].is_object());
    EXPECT_EQ(root["meta"]["dynamic"]["desc"], "persisted");
}

TEST_F(McpAssetsMetaTests, SetAssetDynamicMetadata_UndoRedoViaMcpUndoSystem)
{
    json setParams;
    setParams["asset_id"]  = m_sceneId.ToString();
    setParams["json_path"] = "/dynamic/desc";
    setParams["new_value"] = "after-set";
    Dispatch("assets", "set_asset_dynamic_metadata", setParams);

    ASSERT_EQ(m_sceneAsset->GetDynamicMeta()["desc"], "after-set");

    auto undoRes = Dispatch("undo_history", "Undo");
    ASSERT_TRUE(undoRes["ok"].get<bool>());
    EXPECT_EQ(m_sceneAsset->GetDynamicMeta()["desc"], "initial");
    EXPECT_EQ(m_core->GetAssetDatabase()->GetAssetMeta(m_sceneId)["dynamic"]["desc"], "initial");

    auto redoRes = Dispatch("undo_history", "Redo");
    ASSERT_TRUE(redoRes["ok"].get<bool>());
    EXPECT_EQ(m_sceneAsset->GetDynamicMeta()["desc"], "after-set");
    EXPECT_EQ(m_core->GetAssetDatabase()->GetAssetMeta(m_sceneId)["dynamic"]["desc"], "after-set");
}

TEST_F(McpAssetsMetaTests, SetAssetDynamicMetadata_MissingAssetId_ReturnsError)
{
    auto res = Dispatch("assets", "set_asset_dynamic_metadata",
                        {{"json_path", "/dynamic/desc"}, {"new_value", "x"}});
    EXPECT_FALSE(res["ok"].get<bool>());
}

TEST_F(McpAssetsMetaTests, SetAssetDynamicMetadata_MissingJsonPath_ReturnsError)
{
    auto res = Dispatch("assets", "set_asset_dynamic_metadata",
                        {{"asset_id", m_sceneId.ToString()}, {"new_value", "x"}});
    EXPECT_FALSE(res["ok"].get<bool>());
}

TEST_F(McpAssetsMetaTests, SetAssetDynamicMetadata_MissingNewValue_ReturnsError)
{
    auto res = Dispatch("assets", "set_asset_dynamic_metadata",
                        {{"asset_id", m_sceneId.ToString()}, {"json_path", "/dynamic/desc"}});
    EXPECT_FALSE(res["ok"].get<bool>());
}

TEST_F(McpAssetsMetaTests, SetAssetDynamicMetadata_RejectsStaticPath)
{
    json params;
    params["asset_id"]  = m_sceneId.ToString();
    params["json_path"] = "/static/foo";
    params["new_value"] = "x";
    auto res = Dispatch("assets", "set_asset_dynamic_metadata", params);
    EXPECT_FALSE(res["ok"].get<bool>());
    EXPECT_NE(res["error"].get<std::string>().find("read-only"), std::string::npos);

}

TEST_F(McpAssetsMetaTests, SetAssetDynamicMetadata_RejectsArbitraryPath)
{
    json params;
    params["asset_id"]  = m_sceneId.ToString();
    params["json_path"] = "/foo/bar";
    params["new_value"] = "x";
    auto res = Dispatch("assets", "set_asset_dynamic_metadata", params);
    EXPECT_FALSE(res["ok"].get<bool>());
    EXPECT_NE(res["error"].get<std::string>().find("invalid json_path"), std::string::npos);

}

TEST_F(McpAssetsMetaTests, SetAssetDynamicMetadata_AcceptsDynamicRoot)
{
    json params;
    params["asset_id"]  = m_sceneId.ToString();
    params["json_path"] = "/dynamic";
    params["new_value"] = json{{"desc", "root"}, {"tags", json::array({"t"})}};
    auto res = Dispatch("assets", "set_asset_dynamic_metadata", params);
    ASSERT_TRUE(res["ok"].get<bool>());

}

TEST_F(McpAssetsMetaTests, SetAssetDynamicMetadata_AcceptsDynamicChild)
{
    json params;
    params["asset_id"]  = m_sceneId.ToString();
    params["json_path"] = "/dynamic/desc";
    params["new_value"] = "child-ok";
    auto res = Dispatch("assets", "set_asset_dynamic_metadata", params);
    ASSERT_TRUE(res["ok"].get<bool>());

}

TEST_F(McpAssetsMetaTests, HasStaticMetaSchema_ByClass_TrueForMeshAndTexture)
{
    auto resMesh = Dispatch("assets", "has_static_meta_schema", {{"class", "PA_StaticMesh"}});
    ASSERT_TRUE(resMesh["ok"].get<bool>());
    EXPECT_TRUE(resMesh["has_static_meta_schema"].get<bool>());
    EXPECT_EQ(resMesh["class"], "PA_StaticMesh");

    auto resTex = Dispatch("assets", "has_static_meta_schema", {{"class", "PA_Texture"}});
    ASSERT_TRUE(resTex["ok"].get<bool>());
    EXPECT_TRUE(resTex["has_static_meta_schema"].get<bool>());
}

TEST_F(McpAssetsMetaTests, HasStaticMetaSchema_ByClass_FalseForOtherPATypes)
{
    for (const char* className : {"PA_Material", "PA_Shader", "PA_Skybox", "PA_DScene"})
    {
        auto res = Dispatch("assets", "has_static_meta_schema", {{"class", className}});
        ASSERT_TRUE(res["ok"].get<bool>()) << className;
        EXPECT_FALSE(res["has_static_meta_schema"].get<bool>()) << className;
    }
}

TEST_F(McpAssetsMetaTests, HasStaticMetaSchema_ByAssetId_UsesEntryHeaderClassName)
{
    auto res = Dispatch("assets", "has_static_meta_schema",
                        {{"asset_id", m_sceneId.ToString()}});
    ASSERT_TRUE(res["ok"].get<bool>());
    EXPECT_EQ(res["class"], "PA_DScene");
    EXPECT_FALSE(res["has_static_meta_schema"].get<bool>());
}

TEST_F(McpAssetsMetaTests, HasStaticMetaSchema_UnknownClass_ReturnsFalse)
{
    auto res = Dispatch("assets", "has_static_meta_schema", {{"class", "NotARealClass"}});
    ASSERT_TRUE(res["ok"].get<bool>());
    EXPECT_FALSE(res["has_static_meta_schema"].get<bool>());
}

TEST_F(McpAssetsMetaTests, HasStaticMetaSchema_NoParams_ReturnsError)
{
    auto res = Dispatch("assets", "has_static_meta_schema");
    EXPECT_FALSE(res["ok"].get<bool>());
}
