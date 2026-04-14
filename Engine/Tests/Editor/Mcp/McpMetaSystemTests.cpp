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

TEST_F(McpMetaSystemTests, QueryListOperations_SceneHasExpectedOperations)
{
    auto res = Dispatch("meta", "list_operations");
    ASSERT_TRUE(res["ok"].get<bool>());

    const auto& sceneOps = res["systems"]["scene"];
    ASSERT_TRUE(sceneOps.is_array());

    bool foundGameObjects = false;
    for (const auto& op : sceneOps)
        if (op.get<std::string>() == "game_objects")
            foundGameObjects = true;
    EXPECT_TRUE(foundGameObjects);
}

TEST_F(McpMetaSystemTests, QueryDescribeOperations_ReturnsOk)
{
    json ops = json::array();
    ops.push_back({{"system", "scene"}, {"operation", "game_objects"}});

    auto res = Dispatch("meta", "describe_operations", {{"operations", ops}});
    EXPECT_TRUE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("operations"));
}

TEST_F(McpMetaSystemTests, QueryCapabilities_ReturnsOk)
{
    auto res = Dispatch("meta", "capabilities");
    EXPECT_TRUE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("systems"));
    EXPECT_TRUE(res.contains("commands"));
}
