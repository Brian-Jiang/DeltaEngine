#include "Editor/EditorCoreFixture.h"

#include "Editor/Mcp/McpQueryRouter.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <string>

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

namespace
{

std::string ExecObjectId(EditorCore& core, const std::string& type, json data)
{
    json env;
    env["type"] = type;
    env["data"] = std::move(data);
    return core.ExecuteSerializedCommand(env).value("objectId", std::string{});
}

std::string CreatePointLightObjectId(EditorCore& core, const AssetId& sceneAssetId)
{
    json data;
    data["sceneAssetId"] = sceneAssetId.ToString();
    data["className"]    = "GameObject";
    const std::string goId = ExecObjectId(core, "EditorCommand_CreateGameObject", data);
    if (goId.empty())
        return {};

    json compData;
    compData["sceneAssetId"] = sceneAssetId.ToString();
    compData["gameObjectId"] = goId;
    compData["className"]    = "PointLight";
    return ExecObjectId(core, "EditorCommand_CreateComponent", compData);
}

std::string CreateLegacyGameObject(EditorCore& core, const AssetId& sceneAssetId)
{
    json data;
    data["sceneAssetId"] = sceneAssetId.ToString();
    data["className"]    = "GameObject";
    return ExecObjectId(core, "EditorCommand_CreateGameObject", data);
}

// Routes a command envelope and returns its single synchronous result, verifying
// the request_id is echoed.
json RouteCommand(
    EditorCore& core,
    McpRegistry& reg,
    const char* system,
    const char* command,
    json params,
    const char* requestId)
{
    const McpQueryRouter router(core, reg);
    json env;
    env["type"]        = "command";
    env["system"]      = system;
    env["command"]     = command;
    env["params"]      = std::move(params);
    env["request_id"]  = requestId;

    const json result = json::parse(router.Route(env.dump()));
    EXPECT_EQ(result.value("request_id", std::string{}), requestId);
    return result;
}

} // namespace

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

TEST_F(McpQueryRouterTests, Route_CommandSaveProject_ReturnsResultWithRequestId)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);

    const json out = RouteCommand(*m_core, *reg, "common", "SaveProject",
                                  json::object(), "req-save-1");

    EXPECT_FALSE(out.contains("phase"));
    EXPECT_FALSE(out.contains("queued"));
    EXPECT_TRUE(out["ok"].get<bool>());
    EXPECT_EQ(out["commandType"].get<std::string>(), "SaveDirtyAssets");
}

TEST_F(McpQueryRouterTests, Route_CommandCreateGameObject_ReturnsResultWithObjectId)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);

    const json out = RouteCommand(*m_core, *reg, "scene", "CreateGameObject",
                                  {{"name", "New GameObject"}}, "req-create-7");

    EXPECT_TRUE(out["ok"].get<bool>());
    EXPECT_FALSE(out["objectId"].get<std::string>().empty());
}

TEST_F(McpQueryRouterTests, Route_CommandSelectObject_Sync_ReturnsResult)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);

    const json out = RouteCommand(*m_core, *reg, "selection", "SelectObject",
                                  {{"object_ids", json::array()}}, "req-select-3");

    EXPECT_FALSE(out.contains("phase"));
    EXPECT_FALSE(out.contains("queued"));
    EXPECT_TRUE(out["ok"].get<bool>());
}

TEST_F(McpQueryRouterTests, Route_CommandMissingCommandField_ReturnsErrorPayload)
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

    EXPECT_EQ(out["request_id"].get<std::string>(), "req-bad-1");
    ASSERT_TRUE(out.contains("ok"));
    EXPECT_FALSE(out["ok"].get<bool>());
    ASSERT_TRUE(out["error"].get<std::string>().find("command") != std::string::npos);
}

TEST_F(McpQueryRouterTests, Route_CommandRenameObject_ReturnsResultWithRequestId)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);

    const AssetId sceneAssetId = GetActiveSceneAssetId();
    ASSERT_FALSE(sceneAssetId.IsNull());

    const std::string goId = CreateLegacyGameObject(*m_core, sceneAssetId);
    ASSERT_FALSE(goId.empty());

    const json result = RouteCommand(*m_core, *reg, "common", "RenameObject",
        {{"objectId", goId}, {"newName", "RenamedViaRouter"}}, "req-rename-drain-1");

    EXPECT_TRUE(result["ok"].get<bool>());
    EXPECT_EQ(result["commandType"].get<std::string>(), "EditorCommand_RenameObject");
}

TEST_F(McpQueryRouterTests, Route_CommandCreateGameObjectCustomName_ReturnsResultWithObjectId)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);

    const json result = RouteCommand(*m_core, *reg, "scene", "CreateGameObject",
        {{"name", "CustomNameGO"}}, "req-custom-name-drain");

    EXPECT_TRUE(result["ok"].get<bool>());
    EXPECT_FALSE(result["objectId"].get<std::string>().empty());
    EXPECT_FALSE(result.contains("error"));
}

TEST_F(McpQueryRouterTests, Route_TwoCommands_EachReturnItsOwnResult)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);

    const json resultA = RouteCommand(*m_core, *reg, "scene", "CreateGameObject",
        {{"name", "BatchA"}}, "req-batch-a");
    EXPECT_TRUE(resultA["ok"].get<bool>());
    EXPECT_FALSE(resultA["objectId"].get<std::string>().empty());

    const json resultB = RouteCommand(*m_core, *reg, "scene", "CreateGameObject",
        {{"name", "BatchB"}}, "req-batch-b");
    EXPECT_TRUE(resultB["ok"].get<bool>());
    EXPECT_FALSE(resultB["objectId"].get<std::string>().empty());

    EXPECT_NE(resultA["objectId"].get<std::string>(), resultB["objectId"].get<std::string>());
}

TEST_F(McpQueryRouterTests, Route_SetPositionImmediate_Succeeds)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);

    const AssetId sceneAssetId = GetActiveSceneAssetId();
    ASSERT_FALSE(sceneAssetId.IsNull());
    const std::string plId = CreatePointLightObjectId(*m_core, sceneAssetId);
    ASSERT_FALSE(plId.empty());

    const json result = RouteCommand(*m_core, *reg, "scene", "SetPosition",
        {{"objectId", plId},
         {"value", json::array({1.0f, 0.0f, 0.0f})},
         {"duration_seconds", 0.0f}},
        "req-pos-immediate");
    EXPECT_TRUE(result["ok"].get<bool>());
}
