#include "Editor/Mcp/McpCoreFixture.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class McpMetaSystemTests : public McpCoreFixture {};

TEST_F(McpMetaSystemTests, QueryListOperations_ContainsAllSystems)
{
    auto res = Dispatch("meta", "list_operations");
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res["systems"].is_object());

    for (const char* sys : {"scene", "assets", "selection", "undo_history",
                            "viewport", "common", "reflection", "project", "meta"})
    {
        EXPECT_TRUE(res["systems"].contains(sys)) << "Missing system: " << sys;
    }
}

TEST_F(McpMetaSystemTests, QueryListOperations_SceneHasExpectedQueriesAndCommands)
{
    auto res = Dispatch("meta", "list_operations");
    ASSERT_TRUE(res["ok"].get<bool>());

    const auto& sceneEntry = res["systems"]["scene"];
    ASSERT_TRUE(sceneEntry.is_object());
    ASSERT_TRUE(sceneEntry["queries"].is_array());
    ASSERT_TRUE(sceneEntry["commands"].is_array());

    bool foundQuery = false;
    for (const auto& q : sceneEntry["queries"])
        if (q.get<std::string>() == "game_objects")
            foundQuery = true;
    EXPECT_TRUE(foundQuery);

    bool foundCommand = false;
    for (const auto& c : sceneEntry["commands"])
        if (c.get<std::string>() == "CreateGameObject")
            foundCommand = true;
    EXPECT_TRUE(foundCommand);
}

TEST_F(McpMetaSystemTests, QueryDescribeOperations_ReturnsQueriesAndCommands)
{
    json targets = json::array();
    targets.push_back({{"system", "scene"}, {"query", "game_objects"}});
    targets.push_back({{"system", "scene"}, {"command", "CreateGameObject"}});

    auto res = Dispatch("meta", "describe_operations", {{"targets", targets}});
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res.contains("queries"));
    ASSERT_TRUE(res.contains("commands"));
    EXPECT_TRUE(res["queries"].contains("scene/game_objects"));
    EXPECT_TRUE(res["commands"].contains("scene/CreateGameObject"));
}

TEST_F(McpMetaSystemTests, QueryDescribeOperations_IncludesProjectSettings)
{
    json targets = json::array({{{"system", "project"}, {"query", "settings"}}});

    auto res = Dispatch("meta", "describe_operations", {{"targets", targets}});
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res.contains("queries"));
    EXPECT_TRUE(res["queries"].contains("project/settings"));
}

TEST_F(McpMetaSystemTests, QueryCapabilities_ReturnsOk)
{
    auto res = Dispatch("meta", "capabilities");
    EXPECT_TRUE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("systems"));
}
