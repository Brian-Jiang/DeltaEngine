#include "Editor/Mcp/McpCoreFixture.h"

#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class McpSceneSystemTests : public McpCoreFixture {};

// ─── Queries ─────────────────────────────────────────────────────────────────

TEST_F(McpSceneSystemTests, QueryGameObjects_EmptyWorld_ReturnsEmptyArray)
{
    auto res = Dispatch("scene", "game_objects");
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res["game_objects"].is_array());
    EXPECT_TRUE(res["game_objects"].empty());
}

TEST_F(McpSceneSystemTests, QueryGameObjects_AfterCreate_ListsObject)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    auto res = Dispatch("scene", "game_objects");
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res["game_objects"].is_array());
    ASSERT_EQ(res["game_objects"].size(), 1u);
    EXPECT_EQ(res["game_objects"][0]["object_id"].get<std::string>(), goId);
}

TEST_F(McpSceneSystemTests, QueryGameObject_ByObjectId_ReturnsDetails)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    auto res = Dispatch("scene", "game_object", {{"object_id", goId}});
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res.contains("game_object"));
    EXPECT_EQ(res["game_object"]["object_id"].get<std::string>(), goId);
    EXPECT_TRUE(res["game_object"].contains("components"));
}

TEST_F(McpSceneSystemTests, QueryGameObject_InvalidId_ReturnsError)
{
    auto res = Dispatch("scene", "game_object",
                        {{"object_id", "00000000-0000-0000-0000-000000000000"}});
    EXPECT_FALSE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("error"));
}

TEST_F(McpSceneSystemTests, QueryHierarchy_WithRootObjectId_ReturnsTree)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    // Add PointLight to get a SC id via GetSceneComponents()
    json data;
    data["sceneAssetId"] = GetActiveSceneAssetId().ToString();
    data["gameObjectId"] = goId;
    data["className"]    = "PointLight";
    json env;
    env["type"] = "EditorCommand_CreateComponent";
    env["data"] = data;
    m_core->EnqueueSerializedCommand(env.dump());
    std::vector<std::string> r;
    m_core->DrainCommandQueue(r);
    ASSERT_EQ(r.size(), 1u);
    const std::string plId = json::parse(r[0]).value("objectId", std::string{});
    ASSERT_FALSE(plId.empty());

    auto res = Dispatch("scene", "hierarchy", {{"root_object_id", plId}});
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res.contains("hierarchy"));
    EXPECT_EQ(res["hierarchy"]["object_id"].get<std::string>(), plId);
}

TEST_F(McpSceneSystemTests, QueryComponent_ByObjectId_ReturnsProperties)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    // Add PointLight to have a component accessible via GetSceneComponents()
    json data;
    data["sceneAssetId"] = GetActiveSceneAssetId().ToString();
    data["gameObjectId"] = goId;
    data["className"]    = "PointLight";
    json env;
    env["type"] = "EditorCommand_CreateComponent";
    env["data"] = data;
    m_core->EnqueueSerializedCommand(env.dump());
    std::vector<std::string> r;
    m_core->DrainCommandQueue(r);
    ASSERT_EQ(r.size(), 1u);
    const std::string plId = json::parse(r[0]).value("objectId", std::string{});
    ASSERT_FALSE(plId.empty());

    auto res = Dispatch("scene", "component", {{"object_id", plId}});
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res.contains("component"));
    EXPECT_EQ(res["component"]["class"].get<std::string>(), "PointLight");
}

TEST_F(McpSceneSystemTests, QueryComponentsOnObject_AfterAddingComponent_ListsIt)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    // Add PointLight so there is a known component
    json data;
    data["sceneAssetId"] = GetActiveSceneAssetId().ToString();
    data["gameObjectId"] = goId;
    data["className"]    = "PointLight";
    json env;
    env["type"] = "EditorCommand_CreateComponent";
    env["data"] = data;
    m_core->EnqueueSerializedCommand(env.dump());
    std::vector<std::string> r;
    m_core->DrainCommandQueue(r);
    ASSERT_EQ(r.size(), 1u);
    ASSERT_TRUE(json::parse(r[0])["ok"].get<bool>());

    auto res = Dispatch("scene", "components_on_object", {{"object_id", goId}});
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res["components"].is_array());

    bool foundPointLight = false;
    for (const auto& c : res["components"])
        if (c.value("class", "") == "PointLight")
            foundPointLight = true;
    EXPECT_TRUE(foundPointLight);
}

TEST_F(McpSceneSystemTests, QueryFindByProperty_MatchesCreatedGameObject)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    auto res = Dispatch("scene", "find_by_property",
                        {{"class_name", "GameObject"},
                         {"property_name", "m_name"},
                         {"value", "New GameObject"}});
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res["matches"].is_array());
    ASSERT_GE(res["matches"].size(), 1u);
    EXPECT_EQ(res["matches"][0]["object_id"].get<std::string>(), goId);
}

// ─── Commands ────────────────────────────────────────────────────────────────

TEST_F(McpSceneSystemTests, CommandCreateGameObject_CreatesObject)
{
    auto dispatchRes = Dispatch("scene", "CreateGameObject", {{"name", "New GameObject"}});
    EXPECT_TRUE(dispatchRes["ok"].get<bool>());
    EXPECT_TRUE(dispatchRes.value("queued", false));

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);
    ASSERT_EQ(responses.size(), 1u);
    auto r = json::parse(responses[0]);
    EXPECT_TRUE(r["ok"].get<bool>());
    EXPECT_FALSE(r.value("objectId", std::string{}).empty());

    EXPECT_EQ(m_core->GetWorld()->GetGameObjects().size(), 1u);
}

TEST_F(McpSceneSystemTests, CommandDeleteGameObject_RemovesObject)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());
    ASSERT_EQ(m_core->GetWorld()->GetGameObjects().size(), 1u);

    auto dispatchRes = Dispatch("scene", "DeleteGameObject", {{"objectId", goId}});
    EXPECT_TRUE(dispatchRes["ok"].get<bool>());

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_TRUE(json::parse(responses[0])["ok"].get<bool>());

    EXPECT_EQ(m_core->GetWorld()->GetGameObjects().size(), 0u);
}

TEST_F(McpSceneSystemTests, CommandCreateComponent_AddsPointLight)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    auto dispatchRes = Dispatch("scene", "CreateComponent",
                                {{"objectId", goId}, {"componentClass", "PointLight"}});
    EXPECT_TRUE(dispatchRes["ok"].get<bool>());

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_TRUE(json::parse(responses[0])["ok"].get<bool>());

    // Verify PointLight is present on the GO
    auto compRes = Dispatch("scene", "components_on_object", {{"object_id", goId}});
    bool foundPointLight = false;
    for (const auto& c : compRes["components"])
        if (c.value("class", "") == "PointLight")
            foundPointLight = true;
    EXPECT_TRUE(foundPointLight);
}

TEST_F(McpSceneSystemTests, CommandDeleteComponent_DispatchesDeleteForComponent)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    // Add PointLight
    {
        json data;
        data["sceneAssetId"] = GetActiveSceneAssetId().ToString();
        data["gameObjectId"] = goId;
        data["className"]    = "PointLight";
        json env;
        env["type"] = "EditorCommand_CreateComponent";
        env["data"] = data;
        m_core->EnqueueSerializedCommand(env.dump());
        std::vector<std::string> r;
        m_core->DrainCommandQueue(r);
        ASSERT_EQ(r.size(), 1u);
        ASSERT_TRUE(json::parse(r[0])["ok"].get<bool>());
    }

    // Find PointLight component id
    auto compRes = Dispatch("scene", "components_on_object", {{"object_id", goId}});
    std::string plId;
    for (const auto& c : compRes["components"])
        if (c.value("class", "") == "PointLight")
            plId = c["object_id"].get<std::string>();
    ASSERT_FALSE(plId.empty());

    // The MCP dispatch should queue the delete command (ok:true, queued:true)
    auto dispatchRes = Dispatch("scene", "DeleteComponent", {{"objectId", plId}});
    EXPECT_TRUE(dispatchRes["ok"].get<bool>());
    EXPECT_TRUE(dispatchRes.value("queued", false));

    // Drain to flush pending commands
    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);
    EXPECT_EQ(responses.size(), 1u);
}

TEST_F(McpSceneSystemTests, CommandReparentSceneComponent_ChangesParent)
{
    const std::string goAId = CreateLegacyGameObject();
    const std::string goBId = CreateLegacyGameObject();
    ASSERT_FALSE(goAId.empty());
    ASSERT_FALSE(goBId.empty());

    // Add a PointLight to each GO — these are returned by GetSceneComponents()
    auto addPointLight = [&](const std::string& goId) -> std::string
    {
        json data;
        data["sceneAssetId"] = GetActiveSceneAssetId().ToString();
        data["gameObjectId"] = goId;
        data["className"]    = "PointLight";
        json env;
        env["type"] = "EditorCommand_CreateComponent";
        env["data"] = data;
        m_core->EnqueueSerializedCommand(env.dump());
        std::vector<std::string> r;
        m_core->DrainCommandQueue(r);
        if (r.empty()) return {};
        return json::parse(r[0]).value("objectId", std::string{});
    };

    const std::string plAId = addPointLight(goAId);
    const std::string plBId = addPointLight(goBId);
    ASSERT_FALSE(plAId.empty());
    ASSERT_FALSE(plBId.empty());

    // Reparent PointLight-A under PointLight-B
    auto dispatchRes = Dispatch("scene", "ReparentSceneComponent",
                                {{"objectId", plAId}, {"newParentId", plBId}});
    EXPECT_TRUE(dispatchRes["ok"].get<bool>());

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_TRUE(json::parse(responses[0])["ok"].get<bool>());
}

TEST_F(McpSceneSystemTests, CommandSetTransform_SetsPosition)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    auto dispatchRes = Dispatch("scene", "SetTransform",
                                {{"objectId", goId},
                                 {"position", json::array({1.0f, 2.0f, 3.0f})},
                                 {"scale",    json::array({1.0f, 1.0f, 1.0f})}});
    EXPECT_TRUE(dispatchRes["ok"].get<bool>());

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_TRUE(json::parse(responses[0])["ok"].get<bool>());
}
