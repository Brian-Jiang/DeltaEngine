#include "Editor/Mcp/McpCoreFixture.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class McpProjectSystemTests : public McpCoreFixture {};

TEST_F(McpProjectSystemTests, QueryInfo_HasProjectNameAndVersion)
{
    auto res = Dispatch("project", "info");
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res.contains("project"));
    EXPECT_TRUE(res["project"].contains("name"));
    EXPECT_TRUE(res["project"].contains("engine_version"));
}

TEST_F(McpProjectSystemTests, QuerySettings_ReturnsSettingsObject)
{
    auto res = Dispatch("project", "settings");
    EXPECT_TRUE(res["ok"].get<bool>());
    EXPECT_TRUE(res["settings"].is_object());
}

TEST_F(McpProjectSystemTests, QueryOpenScenes_HasActiveScene)
{
    auto res = Dispatch("project", "open_scenes");
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res["scenes"].is_array());
    ASSERT_GE(res["scenes"].size(), 1u);
    EXPECT_TRUE(res["scenes"][0].contains("asset_id"));
    EXPECT_EQ(res["scenes"][0]["class"].get<std::string>(), "PA_DScene");
}

TEST_F(McpProjectSystemTests, QueryBuildState_HasConfiguration)
{
    auto res = Dispatch("project", "build_state");
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res.contains("build"));
    EXPECT_TRUE(res["build"].contains("configuration"));
}
