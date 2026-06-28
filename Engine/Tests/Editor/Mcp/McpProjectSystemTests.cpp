#include "Editor/Mcp/McpCoreFixture.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Assets/PA_DScene.h"
#include "Runtime/IO/IOManager.h"
#include "Runtime/Settings/EngineSettings.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>
#include <vector>

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

TEST_F(McpProjectSystemTests, CommandLoadScene_MissingParam_ReturnsError)
{
    auto res = Dispatch("project", "LoadScene");
    EXPECT_FALSE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("error"));
}

TEST_F(McpProjectSystemTests, CommandLoadScene_UnknownAssetId_ReturnsError)
{
    auto res = Dispatch("project", "LoadScene",
                        {{"asset_id", "00000000-0000-0000-0000-000000000000"}});
    EXPECT_FALSE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("error"));
}

TEST_F(McpProjectSystemTests, CommandLoadScene_ValidAssetId_SwapsActiveScene)
{
    const AssetId sceneAId = GetActiveSceneAssetId();
    ASSERT_FALSE(sceneAId.IsNull());

    // Create scene B on disk and capture its asset id.
    const std::filesystem::path sceneBPath =
        std::filesystem::weakly_canonical(m_tempDir / "McpProjectSecondScene.dasset.json");
    PA_DScene* sceneB = PA_DScene::Create("McpProjectSecondScene");
    m_core->GetAssetDatabase()->CreateAsset(sceneBPath, sceneB);
    m_core->GetAssetDatabase()->SaveDirtyAssets();
    const AssetId sceneBId = sceneB->GetAssetId();
    ASSERT_FALSE(sceneBId.IsNull());
    ASSERT_NE(sceneBId, sceneAId);

    auto dispatchRes = Dispatch("project", "LoadScene", {{"asset_id", sceneBId.ToString()}});
    EXPECT_TRUE(dispatchRes["ok"].get<bool>());
    EXPECT_TRUE(dispatchRes.value("queued", false));

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);
    ASSERT_EQ(responses.size(), 1u);
    const json reply = json::parse(responses[0]);
    EXPECT_TRUE(reply["ok"].get<bool>());
    EXPECT_EQ(reply.value("commandType", std::string{}), "LoadScene");

    DPrimaryAsset* active = m_core->GetActiveSceneAsset();
    ASSERT_NE(active, nullptr);
    EXPECT_EQ(active->GetAssetId(), sceneBId);
}
