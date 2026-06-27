#include "Editor/Mcp/McpCoreFixture.h"

#include "Runtime/IO/IOManager.h"
#include "Runtime/Settings/EngineSettings.h"

#include <nlohmann/json.hpp>

#include <string>

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

TEST_F(McpProjectSystemTests, QuerySettings_ReturnsLoadedEngineSettings)
{
    const json expected = EngineSettingsToJson(LoadEngineSettings());

    auto res = Dispatch("project", "settings");
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res["settings"].is_object());
    EXPECT_EQ(res["settings"], expected);
    EXPECT_EQ(res["settings"]["version"].get<uint32_t>(), expected["version"].get<uint32_t>());
    EXPECT_EQ(res["settings"]["graphics"]["renderPath"].get<std::string>(),
        expected["graphics"]["renderPath"].get<std::string>());
    EXPECT_EQ(res["settings"]["graphics"]["vsync"].get<bool>(),
        expected["graphics"]["vsync"].get<bool>());

    const auto& shadowAtlas = res["settings"]["graphics"]["shadowAtlas"];
    ASSERT_TRUE(shadowAtlas.is_object());
    EXPECT_TRUE(shadowAtlas.contains("atlasSize"));
    EXPECT_TRUE(shadowAtlas.contains("directionalTileSize"));
    EXPECT_TRUE(shadowAtlas.contains("spotTileSize"));
    EXPECT_TRUE(shadowAtlas.contains("pointFaceSize"));
    EXPECT_TRUE(shadowAtlas.contains("pointCubeCount"));

    ASSERT_TRUE(res.contains("settingsFilePath"));
    const std::string settingsFilePath = res["settingsFilePath"].get<std::string>();
    EXPECT_FALSE(settingsFilePath.empty());
    EXPECT_EQ(settingsFilePath, IOManager::GetEngineSettingsPath().string());
    EXPECT_TRUE(settingsFilePath.ends_with("Settings/EngineSettings.json")
        || settingsFilePath.ends_with("Settings\\EngineSettings.json"));
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
