#include "Editor/Mcp/McpCoreFixture.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class McpRegistryDispatchTests : public McpCoreFixture {};

TEST_F(McpRegistryDispatchTests, DispatchQuery_UnknownSystem_ReturnsHint)
{
    const json res =
        DispatchQuery("not_a_registered_system_xyz", "any_op", json::object());

    ASSERT_TRUE(res.contains("ok"));
    EXPECT_FALSE(res["ok"].get<bool>());
    ASSERT_TRUE(res.contains("hint"));
    ASSERT_TRUE(res["error"].get<std::string>().find("Unknown system") != std::string::npos);
}

TEST_F(McpRegistryDispatchTests, DispatchQuery_UnknownQuery_ReturnsHint)
{
    const json res = DispatchQuery("meta", "nonexistent_query_xyz", json::object());

    ASSERT_TRUE(res.contains("ok"));
    EXPECT_FALSE(res["ok"].get<bool>());
    ASSERT_TRUE(res.contains("hint"));
}

TEST_F(McpRegistryDispatchTests, DispatchCommand_UnknownCommand_ReturnsHint)
{
    const json res = DispatchCommand("scene", "nonexistent_command_xyz", json::object());

    ASSERT_TRUE(res.contains("ok"));
    EXPECT_FALSE(res["ok"].get<bool>());
    ASSERT_TRUE(res.contains("hint"));
}

TEST_F(McpRegistryDispatchTests, HasQuery_KnownVersusUnknown)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);

    EXPECT_TRUE(reg->HasQuery("meta", "list_operations"));
    EXPECT_FALSE(reg->HasQuery("meta", "nonexistent_query_xyz"));
    EXPECT_FALSE(reg->HasQuery("not_a_system", "anything"));
}

TEST_F(McpRegistryDispatchTests, HasCommand_KnownVersusUnknown)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);

    EXPECT_TRUE(reg->HasCommand("scene", "CreateGameObject"));
    EXPECT_FALSE(reg->HasCommand("scene", "nonexistent_command_xyz"));
    EXPECT_FALSE(reg->HasCommand("not_a_system", "anything"));
}

TEST_F(McpRegistryDispatchTests, GetQueryNames_UnknownSystem_ReturnsEmpty)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);

    EXPECT_TRUE(reg->GetQueryNames("not_a_registered_system_xyz").empty());
    EXPECT_TRUE(reg->GetCommandNames("not_a_registered_system_xyz").empty());
}
