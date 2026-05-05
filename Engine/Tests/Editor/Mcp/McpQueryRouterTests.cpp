#include "Editor/EditorCoreFixture.h"

#include "Editor/Mcp/McpQueryRouter.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <string>

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class McpQueryRouterTests : public EditorCoreFixture {};

TEST_F(McpQueryRouterTests, Route_MetaList_ValidEnvelope_ReturnsOk)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);
    const McpQueryRouter router(*m_core, *reg);

    json env;
    env["type"]      = "query";
    env["system"]    = "meta";
    env["operation"] = "list_operations";
    env["params"]    = json::object();

    const json out = json::parse(router.Route(env.dump()));

    ASSERT_TRUE(out.contains("ok"));
    EXPECT_TRUE(out["ok"].get<bool>());
}

TEST_F(McpQueryRouterTests, Route_EmptyEnvelope_ReturnsErrorPayload)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);
    const McpQueryRouter router(*m_core, *reg);

    const json out = json::parse(router.Route(std::string{}));

    ASSERT_TRUE(out.contains("ok"));
    EXPECT_FALSE(out["ok"].get<bool>());
    ASSERT_TRUE(out.contains("error"));
}

TEST_F(McpQueryRouterTests, Route_MalformedJson_ReturnsParseErrorPayload)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);
    const McpQueryRouter router(*m_core, *reg);

    const json out = json::parse(router.Route("{"));

    ASSERT_TRUE(out.contains("ok"));
    EXPECT_FALSE(out["ok"].get<bool>());
    ASSERT_TRUE(out["error"].get<std::string>().find("JSON parse error") != std::string::npos);
}

TEST_F(McpQueryRouterTests, Route_QueryMissingSystem_ReturnsErrorPayload)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);
    const McpQueryRouter router(*m_core, *reg);

    json env;
    env["type"]       = "query";
    env["operation"]  = "list_operations";
    env["params"]     = json::object();

    const json out = json::parse(router.Route(env.dump()));

    ASSERT_TRUE(out.contains("ok"));
    EXPECT_FALSE(out["ok"].get<bool>());
}

TEST_F(McpQueryRouterTests, Route_CommandSaveProject_ValidEnvelope_ReturnsOk)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);
    const McpQueryRouter router(*m_core, *reg);

    json env;
    env["type"]     = "command";
    env["system"]   = "common";
    env["command"]  = "SaveProject";
    env["params"]   = json::object();

    const json out = json::parse(router.Route(env.dump()));

    ASSERT_TRUE(out.contains("ok"));
    EXPECT_TRUE(out["ok"].get<bool>());
    ASSERT_TRUE(out.contains("queued"));
    EXPECT_TRUE(out["queued"].get<bool>());
}

TEST_F(McpQueryRouterTests, Route_CommandMissingCommandField_ReturnsErrorPayload)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);
    const McpQueryRouter router(*m_core, *reg);

    json env;
    env["type"]    = "command";
    env["system"]  = "common";
    env["params"]  = json::object();

    const json out = json::parse(router.Route(env.dump()));

    ASSERT_TRUE(out.contains("ok"));
    EXPECT_FALSE(out["ok"].get<bool>());
    ASSERT_TRUE(out["error"].get<std::string>().find("command") != std::string::npos);
}
