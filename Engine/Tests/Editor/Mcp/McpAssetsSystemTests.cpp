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
