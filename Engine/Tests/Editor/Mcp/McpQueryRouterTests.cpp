#include "Editor/EditorCoreFixture.h"

#include "Editor/Mcp/McpQueryRouter.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <string>
#include <vector>

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

namespace
{

std::string CreatePointLightObjectId(EditorCore& core, const AssetId& sceneAssetId)
{
    json data;
    data["sceneAssetId"] = sceneAssetId.ToString();
    data["className"]    = "GameObject";
    json createEnv;
    createEnv["type"] = "EditorCommand_CreateGameObject";
    createEnv["data"] = data;
    core.EnqueueSerializedCommand(createEnv.dump());
    std::vector<std::string> createResponses;
    core.DrainCommandQueue(createResponses);
    if (createResponses.empty())
        return {};
    const std::string goId = json::parse(createResponses[0]).value("objectId", std::string{});
    if (goId.empty())
        return {};

    json compData;
    compData["sceneAssetId"] = sceneAssetId.ToString();
    compData["gameObjectId"] = goId;
    compData["className"]    = "PointLight";
    json compEnv;
    compEnv["type"] = "EditorCommand_CreateComponent";
    compEnv["data"] = compData;
    core.EnqueueSerializedCommand(compEnv.dump());
    std::vector<std::string> compResponses;
    core.DrainCommandQueue(compResponses);
    if (compResponses.empty())
        return {};
    return json::parse(compResponses[0]).value("objectId", std::string{});
}

void RouteAndExpectExpectsResult(
    EditorCore& core,
    McpRegistry& reg,
    const char* system,
    const char* command,
    json params,
    const char* requestId,
    bool expectedExpectsResult)
{
    const McpQueryRouter router(core, reg);
    json env;
    env["type"]        = "command";
    env["system"]      = system;
    env["command"]     = command;
    env["params"]      = std::move(params);
    env["request_id"]  = requestId;

    const json accept = json::parse(router.Route(env.dump()));
    EXPECT_EQ(accept["phase"].get<std::string>(), "accept") << command;
    EXPECT_EQ(accept["request_id"].get<std::string>(), requestId) << command;
    ASSERT_TRUE(accept.contains("expects_result")) << command;
    EXPECT_EQ(accept["expects_result"].get<bool>(), expectedExpectsResult) << command;
}

json RouteCommandAndDrain(
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

    const json accept = json::parse(router.Route(env.dump()));
    EXPECT_EQ(accept["phase"].get<std::string>(), "accept");
    EXPECT_EQ(accept["request_id"].get<std::string>(), requestId);

    std::vector<std::string> responses;
    core.DrainCommandQueue(responses);
    EXPECT_EQ(responses.size(), 1u);
    const json result = json::parse(responses[0]);
    EXPECT_EQ(result["phase"].get<std::string>(), "result");
    EXPECT_EQ(result["request_id"].get<std::string>(), requestId);
    return result;
}

std::string CreateLegacyGameObject(EditorCore& core, const AssetId& sceneAssetId)
{
    json data;
    data["sceneAssetId"] = sceneAssetId.ToString();
    data["className"]    = "GameObject";
    json createEnv;
    createEnv["type"] = "EditorCommand_CreateGameObject";
    createEnv["data"] = data;
    core.EnqueueSerializedCommand(createEnv.dump());
    std::vector<std::string> createResponses;
    core.DrainCommandQueue(createResponses);
    if (createResponses.empty())
        return {};
    return json::parse(createResponses[0]).value("objectId", std::string{});
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

TEST_F(McpQueryRouterTests, Route_Commands_AcceptExpectsResultMatchesPolicy)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);

    const AssetId sceneAssetId = GetActiveSceneAssetId();
    ASSERT_FALSE(sceneAssetId.IsNull());

    json createData;
    createData["sceneAssetId"] = sceneAssetId.ToString();
    createData["className"]    = "GameObject";
    json createEnv;
    createEnv["type"] = "EditorCommand_CreateGameObject";
    createEnv["data"] = createData;
    m_core->EnqueueSerializedCommand(createEnv.dump());
    std::vector<std::string> createResponses;
    m_core->DrainCommandQueue(createResponses);
    ASSERT_EQ(createResponses.size(), 1u);
    const std::string goId = json::parse(createResponses[0]).value("objectId", std::string{});
    ASSERT_FALSE(goId.empty());

    const std::string plId = CreatePointLightObjectId(*m_core, sceneAssetId);
    ASSERT_FALSE(plId.empty());

    RouteAndExpectExpectsResult(*m_core, *reg, "common", "RenameObject",
        {{"objectId", goId}, {"newName", "RenamedGO"}},
        "req-rename-er", true);

    RouteAndExpectExpectsResult(*m_core, *reg, "common", "SaveProject",
        json::object(), "req-save-er", false);

    RouteAndExpectExpectsResult(*m_core, *reg, "scene", "LoadScene",
        {{"scenePath", m_defaultScenePath.generic_string()}},
        "req-load-er", false);

    RouteAndExpectExpectsResult(*m_core, *reg, "scene", "SetPosition",
        {{"objectId", plId},
         {"value", json::array({1.0f, 0.0f, 0.0f})},
         {"duration_seconds", 0.0f}},
        "req-pos-immediate-er", true);

    RouteAndExpectExpectsResult(*m_core, *reg, "scene", "SetPosition",
        {{"objectId", plId},
         {"value", json::array({1.0f, 0.0f, 0.0f})},
         {"duration_seconds", 1.0f}},
        "req-pos-animated-er", false);

    RouteAndExpectExpectsResult(*m_core, *reg, "lights", "SetIntensity",
        {{"assetId", sceneAssetId.ToString()},
         {"objectId", plId},
         {"value", 2.0f},
         {"duration_seconds", 0.0f}},
        "req-light-immediate-er", true);

    RouteAndExpectExpectsResult(*m_core, *reg, "lights", "SetIntensity",
        {{"assetId", sceneAssetId.ToString()},
         {"objectId", plId},
         {"value", 2.0f},
         {"duration_seconds", 1.0f}},
        "req-light-animated-er", false);

    RouteAndExpectExpectsResult(*m_core, *reg, "undo_history", "Undo",
        json::object(), "req-undo-er", false);

    RouteAndExpectExpectsResult(*m_core, *reg, "selection", "SelectObject",
        {{"object_ids", json::array()}},
        "req-select-er", false);

    RouteAndExpectExpectsResult(*m_core, *reg, "assets", "reimport_assets",
        {{"asset_ids", json::array({sceneAssetId.ToString()})}},
        "req-reimport-er", true);
}

TEST_F(McpQueryRouterTests, Route_CommandRenameObject_Drain_ReturnsResultWithRequestId)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);

    const AssetId sceneAssetId = GetActiveSceneAssetId();
    ASSERT_FALSE(sceneAssetId.IsNull());

    const std::string goId = CreateLegacyGameObject(*m_core, sceneAssetId);
    ASSERT_FALSE(goId.empty());

    const json result = RouteCommandAndDrain(
        *m_core,
        *reg,
        "common",
        "RenameObject",
        {{"objectId", goId}, {"newName", "RenamedViaRouter"}},
        "req-rename-drain-1");

    EXPECT_TRUE(result["ok"].get<bool>());
    EXPECT_EQ(result["commandType"].get<std::string>(), "EditorCommand_RenameObject");
}

TEST_F(McpQueryRouterTests, Route_CommandCreateGameObjectCustomName_Drain_ReturnsResultWithObjectId)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);

    const json result = RouteCommandAndDrain(
        *m_core,
        *reg,
        "scene",
        "CreateGameObject",
        {{"name", "CustomNameGO"}},
        "req-custom-name-drain");

    EXPECT_TRUE(result["ok"].get<bool>());
    EXPECT_FALSE(result["objectId"].get<std::string>().empty());
    EXPECT_FALSE(result.contains("error"));
}

TEST_F(McpQueryRouterTests, Route_TwoQueuedCommands_OneDrain_ReturnsBothResultsWithRequestIds)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);
    const McpQueryRouter router(*m_core, *reg);

    json envA;
    envA["type"]        = "command";
    envA["system"]      = "scene";
    envA["command"]     = "CreateGameObject";
    envA["params"]      = {{"name", "BatchA"}};
    envA["request_id"]  = "req-batch-a";

    json envB;
    envB["type"]        = "command";
    envB["system"]      = "scene";
    envB["command"]     = "CreateGameObject";
    envB["params"]      = {{"name", "BatchB"}};
    envB["request_id"]  = "req-batch-b";

    const json acceptA = json::parse(router.Route(envA.dump()));
    EXPECT_EQ(acceptA["phase"].get<std::string>(), "accept");
    EXPECT_EQ(acceptA["request_id"].get<std::string>(), "req-batch-a");
    EXPECT_TRUE(acceptA["queued"].get<bool>());

    const json acceptB = json::parse(router.Route(envB.dump()));
    EXPECT_EQ(acceptB["phase"].get<std::string>(), "accept");
    EXPECT_EQ(acceptB["request_id"].get<std::string>(), "req-batch-b");
    EXPECT_TRUE(acceptB["queued"].get<bool>());

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);

    ASSERT_EQ(responses.size(), 2u);
    const json resultA = json::parse(responses[0]);
    const json resultB = json::parse(responses[1]);
    EXPECT_EQ(resultA["phase"].get<std::string>(), "result");
    EXPECT_EQ(resultA["request_id"].get<std::string>(), "req-batch-a");
    EXPECT_TRUE(resultA["ok"].get<bool>());
    EXPECT_FALSE(resultA["objectId"].get<std::string>().empty());

    EXPECT_EQ(resultB["phase"].get<std::string>(), "result");
    EXPECT_EQ(resultB["request_id"].get<std::string>(), "req-batch-b");
    EXPECT_TRUE(resultB["ok"].get<bool>());
    EXPECT_FALSE(resultB["objectId"].get<std::string>().empty());
}

TEST_F(McpQueryRouterTests, Route_SyncSelectObject_Drain_EmitsNoResults)
{
    auto* reg = m_core->GetMcpRegistry();
    ASSERT_NE(reg, nullptr);
    const McpQueryRouter router(*m_core, *reg);

    json env;
    env["type"]        = "command";
    env["system"]      = "selection";
    env["command"]     = "SelectObject";
    env["params"]      = {{"object_ids", json::array()}};
    env["request_id"]  = "req-select-drain";

    const json accept = json::parse(router.Route(env.dump()));
    EXPECT_EQ(accept["phase"].get<std::string>(), "accept");
    EXPECT_FALSE(accept["expects_result"].get<bool>());

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);
    EXPECT_TRUE(responses.empty());
}
