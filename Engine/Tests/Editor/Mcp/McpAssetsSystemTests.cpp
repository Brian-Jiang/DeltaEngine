#include "Editor/Mcp/McpCoreFixture.h"

#include <nlohmann/json.hpp>

#include <string>

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class McpAssetsSystemTests : public McpCoreFixture {};

TEST_F(McpAssetsSystemTests, QueryList_ReturnsOkWithAssetsArray)
{
    auto res = Dispatch("assets", "list", {{"folder", "/"}, {"recursive", true}});
    EXPECT_TRUE(res["ok"].get<bool>());
    EXPECT_TRUE(res["assets"].is_array());
}

TEST_F(McpAssetsSystemTests, QueryGet_WithSceneAssetId_ReturnsDetails)
{
    const AssetId sceneId = GetActiveSceneAssetId();
    ASSERT_FALSE(sceneId.IsNull());

    auto res = Dispatch("assets", "get", {{"asset_id", sceneId.ToString()}});
    EXPECT_TRUE(res["ok"].get<bool>());
    EXPECT_EQ(res["asset_id"].get<std::string>(), sceneId.ToString());
    EXPECT_EQ(res["type"].get<std::string>(), "scene");

    ASSERT_TRUE(res["objects"].is_array());
    EXPECT_FALSE(res["objects"].empty());
    for (const auto& obj : res["objects"])
    {
        EXPECT_TRUE(obj.contains("object_id"));
        EXPECT_TRUE(obj["object_id"].is_string());
        EXPECT_FALSE(obj["object_id"].get<std::string>().empty());
        EXPECT_TRUE(obj.contains("class"));
        EXPECT_TRUE(obj["class"].is_string());
        EXPECT_FALSE(obj["class"].get<std::string>().empty());
    }
}

TEST_F(McpAssetsSystemTests, QueryGet_ObjectsAvailableWithoutPropertiesField)
{
    const AssetId sceneId = GetActiveSceneAssetId();
    ASSERT_FALSE(sceneId.IsNull());

    auto res = Dispatch("assets", "get",
                        {{"asset_id", sceneId.ToString()},
                         {"include_fields", json::array({"dependencies"})}});
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res["objects"].is_array());
    EXPECT_FALSE(res["objects"].empty());
    EXPECT_FALSE(res.contains("properties"));
}

TEST_F(McpAssetsSystemTests, QueryGet_InvalidId_ReturnsError)
{
    auto res = Dispatch("assets", "get",
                        {{"asset_id", "00000000-0000-0000-0000-000000000000"}});
    EXPECT_FALSE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("error"));
}

TEST_F(McpAssetsSystemTests, QuerySearch_ReturnsOkWithResultsArray)
{
    auto res = Dispatch("assets", "search", {{"query", "DefaultScene"}});
    EXPECT_TRUE(res["ok"].get<bool>());
    EXPECT_TRUE(res["results"].is_array());
}

TEST_F(McpAssetsSystemTests, QueryFolderTree_ReturnsOkWithTreeNode)
{
    auto res = Dispatch("assets", "folder_tree");
    EXPECT_TRUE(res.contains("ok"));
    // Tree returns ok:true with a tree node, or ok:false if asset root not found
    if (res["ok"].get<bool>())
    {
        EXPECT_TRUE(res.contains("tree"));
        EXPECT_TRUE(res["tree"].contains("name"));
    }
}

TEST_F(McpAssetsSystemTests, QueryUsages_WithSceneAssetId_ReturnsUsagesArray)
{
    const AssetId sceneId = GetActiveSceneAssetId();
    ASSERT_FALSE(sceneId.IsNull());

    auto res = Dispatch("assets", "usages", {{"asset_id", sceneId.ToString()}});
    EXPECT_TRUE(res["ok"].get<bool>());
    EXPECT_TRUE(res["usages"].is_array());
}

// reimport_assets: the command validates + enqueues; DrainCommandQueue runs the actual
// reimport on the main thread. The end-to-end shader-recompile path needs a compilable
// .slang source + GPU, so these tests cover routing/validation and non-shader handling only.

TEST_F(McpAssetsSystemTests, ReimportAssets_MissingParam_ReturnsError)
{
    auto res = Dispatch("assets", "reimport_assets");
    EXPECT_FALSE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("error"));
}

TEST_F(McpAssetsSystemTests, ReimportAssets_EmptyArray_ReturnsError)
{
    auto res = Dispatch("assets", "reimport_assets", {{"asset_ids", json::array()}});
    EXPECT_FALSE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("error"));
}

TEST_F(McpAssetsSystemTests, ReimportAssets_NonShaderAsset_QueuedAndSkipped)
{
    const AssetId sceneId = GetActiveSceneAssetId();
    ASSERT_FALSE(sceneId.IsNull());

    auto res = Dispatch("assets", "reimport_assets",
                        {{"asset_ids", json::array({sceneId.ToString()})}});
    ASSERT_TRUE(res["ok"].get<bool>());
    EXPECT_TRUE(res.value("queued", false));

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);
    ASSERT_EQ(responses.size(), 1u);

    const json drained = json::parse(responses[0]);
    EXPECT_TRUE(drained["ok"].get<bool>());
    ASSERT_TRUE(drained["skipped"].is_array());
    ASSERT_EQ(drained["skipped"].size(), 1u);
    EXPECT_EQ(drained["skipped"][0]["asset_id"].get<std::string>(), sceneId.ToString());
    EXPECT_TRUE(drained["reimported"].empty());
}

TEST_F(McpAssetsSystemTests, ReimportAssets_UnknownId_QueuedAndSkipped)
{
    const std::string unknownId = "11111111-1111-1111-1111-111111111111";
    auto res = Dispatch("assets", "reimport_assets",
                        {{"asset_ids", json::array({unknownId})}});
    ASSERT_TRUE(res["ok"].get<bool>());

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);
    ASSERT_EQ(responses.size(), 1u);

    const json drained = json::parse(responses[0]);
    EXPECT_TRUE(drained["ok"].get<bool>());
    ASSERT_EQ(drained["skipped"].size(), 1u);
    EXPECT_EQ(drained["skipped"][0]["asset_id"].get<std::string>(), unknownId);
}
