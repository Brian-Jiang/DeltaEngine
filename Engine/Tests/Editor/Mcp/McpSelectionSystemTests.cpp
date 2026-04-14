#include "Editor/Mcp/McpCoreFixture.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class McpSelectionSystemTests : public McpCoreFixture {};

TEST_F(McpSelectionSystemTests, QueryCurrent_InitiallyEmpty)
{
    auto res = Dispatch("selection", "current");
    EXPECT_TRUE(res["ok"].get<bool>());
    EXPECT_TRUE(res["game_objects"].is_array());
    EXPECT_TRUE(res["game_objects"].empty());
    EXPECT_TRUE(res["components"].is_array());
    EXPECT_TRUE(res["assets"].is_array());
}

TEST_F(McpSelectionSystemTests, CommandSelectObject_SelectsGameObject)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    auto res = Dispatch("selection", "SelectObject",
                        {{"object_ids", json::array({goId})}});
    EXPECT_TRUE(res["ok"].get<bool>());
    EXPECT_EQ(res["count"].get<int>(), 1);

    auto cur = Dispatch("selection", "current");
    EXPECT_TRUE(cur["ok"].get<bool>());
    ASSERT_EQ(cur["game_objects"].size(), 1u);
    EXPECT_EQ(cur["game_objects"][0]["object_id"].get<std::string>(), goId);
}

TEST_F(McpSelectionSystemTests, CommandSelectObject_EmptyList_ClearsSelection)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    Dispatch("selection", "SelectObject", {{"object_ids", json::array({goId})}});
    ASSERT_EQ(Dispatch("selection", "current")["game_objects"].size(), 1u);

    auto res = Dispatch("selection", "SelectObject",
                        {{"object_ids", json::array()}});
    EXPECT_TRUE(res["ok"].get<bool>());
    EXPECT_TRUE(Dispatch("selection", "current")["game_objects"].empty());
}
