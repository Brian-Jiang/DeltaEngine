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
    env["type"]   = "query";
    env["system"] = "meta";
    env["query"]  = "list_operations";
    env["params"] = json::object();

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
    env["type"]    = "query";
    env["query"]   = "list_operations";
    env["params"]  = json::object();

    const json out = json::parse(router.Route(env.dump()));

    ASSERT_TRUE(out.contains("ok"));
    EXPECT_FALSE(out["ok"].get<bool>());
}

TEST_F(McpQueryRouterTests, Route_CommandSaveProject_ValidEnvelope_ReturnsAccept)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);
    const McpQueryRouter router(*m_core, *reg);

    json env;
    env["type"]        = "command";
    env["system"]      = "common";
    env["command"]     = "SaveProject";
    env["params"]      = json::object();
    env["request_id"]  = "req-save-1";

    const json out = json::parse(router.Route(env.dump()));

    EXPECT_EQ(out["phase"].get<std::string>(), "accept");
    EXPECT_EQ(out["request_id"].get<std::string>(), "req-save-1");
    ASSERT_TRUE(out.contains("ok"));
    EXPECT_TRUE(out["ok"].get<bool>());
    ASSERT_TRUE(out.contains("queued"));
    EXPECT_TRUE(out["queued"].get<bool>());
    ASSERT_TRUE(out.contains("expects_result"));
    EXPECT_FALSE(out["expects_result"].get<bool>());
}

TEST_F(McpQueryRouterTests, Route_CommandSaveProject_Drain_ReturnsResultWithRequestId)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);
    const McpQueryRouter router(*m_core, *reg);

    json env;
    env["type"]        = "command";
    env["system"]      = "common";
    env["command"]     = "SaveProject";
    env["params"]      = json::object();
    env["request_id"]  = "req-save-1";

    const json accept = json::parse(router.Route(env.dump()));
    EXPECT_EQ(accept["phase"].get<std::string>(), "accept");
    EXPECT_TRUE(accept["queued"].get<bool>());

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);

    ASSERT_EQ(responses.size(), 1u);
    const json result = json::parse(responses[0]);
    EXPECT_EQ(result["phase"].get<std::string>(), "result");
    EXPECT_EQ(result["request_id"].get<std::string>(), "req-save-1");
    EXPECT_TRUE(result["ok"].get<bool>());
    EXPECT_EQ(result["commandType"].get<std::string>(), "SaveDirtyAssets");
}

TEST_F(McpQueryRouterTests, Route_CommandCreateGameObject_Drain_ReturnsResultWithRequestId)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);
    const McpQueryRouter router(*m_core, *reg);

    json env;
    env["type"]        = "command";
    env["system"]      = "scene";
    env["command"]     = "CreateGameObject";
    env["params"]      = {{"name", "New GameObject"}};
    env["request_id"]  = "req-create-7";

    const json accept = json::parse(router.Route(env.dump()));
    EXPECT_EQ(accept["phase"].get<std::string>(), "accept");
    EXPECT_TRUE(accept["queued"].get<bool>());

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);

    ASSERT_EQ(responses.size(), 1u);
    const json result = json::parse(responses[0]);
    EXPECT_EQ(result["phase"].get<std::string>(), "result");
    EXPECT_EQ(result["request_id"].get<std::string>(), "req-create-7");
    EXPECT_TRUE(result["ok"].get<bool>());
    EXPECT_FALSE(result["objectId"].get<std::string>().empty());
}

TEST_F(McpQueryRouterTests, Route_CommandCreateGameObject_WithRequestId_ReturnsAccept)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);
    const McpQueryRouter router(*m_core, *reg);

    json env;
    env["type"]        = "command";
    env["system"]      = "scene";
    env["command"]     = "CreateGameObject";
    env["params"]      = {{"name", "New GameObject"}};
    env["request_id"]  = "req-create-7";

    const json out = json::parse(router.Route(env.dump()));

    EXPECT_EQ(out["phase"].get<std::string>(), "accept");
    EXPECT_EQ(out["request_id"].get<std::string>(), "req-create-7");
    ASSERT_TRUE(out.contains("ok"));
    EXPECT_TRUE(out["ok"].get<bool>());
    ASSERT_TRUE(out.contains("expects_result"));
    EXPECT_TRUE(out["expects_result"].get<bool>());
    ASSERT_TRUE(out.contains("queued"));
    EXPECT_TRUE(out["queued"].get<bool>());
}

TEST_F(McpQueryRouterTests, Route_CommandSelectObject_Sync_ReturnsAcceptWithoutQueued)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);
    const McpQueryRouter router(*m_core, *reg);

    json env;
    env["type"]        = "command";
    env["system"]      = "selection";
    env["command"]     = "SelectObject";
    env["params"]      = {{"object_ids", json::array()}};
    env["request_id"]  = "req-select-3";

    const json out = json::parse(router.Route(env.dump()));

    EXPECT_EQ(out["phase"].get<std::string>(), "accept");
    EXPECT_EQ(out["request_id"].get<std::string>(), "req-select-3");
    ASSERT_TRUE(out.contains("ok"));
    EXPECT_TRUE(out["ok"].get<bool>());
    ASSERT_TRUE(out.contains("expects_result"));
    EXPECT_FALSE(out["expects_result"].get<bool>());
    EXPECT_FALSE(out.contains("queued"));
}

TEST_F(McpQueryRouterTests, Route_CommandMissingCommandField_ReturnsAcceptErrorPayload)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);
    const McpQueryRouter router(*m_core, *reg);

    json env;
    env["type"]        = "command";
    env["system"]      = "common";
    env["params"]      = json::object();
    env["request_id"]  = "req-bad-1";

    const json out = json::parse(router.Route(env.dump()));

    EXPECT_EQ(out["phase"].get<std::string>(), "accept");
    EXPECT_EQ(out["request_id"].get<std::string>(), "req-bad-1");
    ASSERT_TRUE(out.contains("ok"));
    EXPECT_FALSE(out["ok"].get<bool>());
    ASSERT_TRUE(out.contains("expects_result"));
    EXPECT_FALSE(out["expects_result"].get<bool>());
    ASSERT_TRUE(out["error"].get<std::string>().find("command") != std::string::npos);
}
