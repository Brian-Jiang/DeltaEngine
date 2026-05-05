#include "Editor/Mcp/McpCoreFixture.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class McpRegistryDispatchTests : public McpCoreFixture {};

TEST_F(McpRegistryDispatchTests, Dispatch_UnknownSystem_ReturnsHint)
{
    const json res =
        Dispatch("not_a_registered_system_xyz", "any_op", json::object());

    ASSERT_TRUE(res.contains("ok"));
    EXPECT_FALSE(res["ok"].get<bool>());
    ASSERT_TRUE(res.contains("hint"));
    ASSERT_TRUE(res["error"].get<std::string>().find("Unknown system") != std::string::npos);
}

TEST_F(McpRegistryDispatchTests, Dispatch_UnknownOperation_ReturnsHint)
{
    const json res = Dispatch("meta", "nonexistent_operation_xyz", json::object());

    ASSERT_TRUE(res.contains("ok"));
    EXPECT_FALSE(res["ok"].get<bool>());
    ASSERT_TRUE(res.contains("hint"));
}

TEST_F(McpRegistryDispatchTests, HasOperation_KnownVersusUnknown)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);

    EXPECT_TRUE(reg->HasOperation("meta", "list_operations"));
    EXPECT_FALSE(reg->HasOperation("meta", "nonexistent_operation_xyz"));
    EXPECT_FALSE(reg->HasOperation("not_a_system", "anything"));
}

TEST_F(McpRegistryDispatchTests, GetOperationNames_UnknownSystem_ReturnsEmpty)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);

    const auto names = reg->GetOperationNames("not_a_registered_system_xyz");
    EXPECT_TRUE(names.empty());
}
