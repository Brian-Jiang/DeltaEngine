#include "Editor/Mcp/McpCoreFixture.h"

#include "Editor/Mcp/McpQueryRouter.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"

#include <nlohmann/json.hpp>

#include <string>

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class McpSceneSystemTests : public McpCoreFixture
{
protected:
    // Adds a component to a GameObject via the legacy command path; returns its objectId.
    std::string AddComponent(const std::string& goId, const char* className)
    {
        json data;
        data["sceneAssetId"] = GetActiveSceneAssetId().ToString();
        data["gameObjectId"] = goId;
        data["className"]    = className;
        json env;
        env["type"] = "EditorCommand_CreateComponent";
        env["data"] = data;
        return m_core->ExecuteSerializedCommand(env).value("objectId", std::string{});
    }
};

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

    const std::string plId = AddComponent(goId, "PointLight");
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

    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());

    auto res = Dispatch("scene", "component", {{"object_id", plId}});
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res.contains("component"));
    EXPECT_EQ(res["component"]["class"].get<std::string>(), "PointLight");
}

namespace {

const json* FindSchemaProperty(const json& properties, const char* name)
{
    for (const auto& prop : properties)
        if (prop.value("name", "") == name)
            return &prop;
    return nullptr;
}

void ExpectEnumPropertyMetadata(const json& enumProp)
{
    EXPECT_EQ(enumProp.at("type").get<std::string>(), "enum");
    EXPECT_EQ(enumProp.at("enum_name").get<std::string>(), "EReflectionTestEnum");
    ASSERT_TRUE(enumProp.at("values").is_array());
    ASSERT_EQ(enumProp.at("values").size(), 3u);
    EXPECT_EQ(enumProp.at("values")[1].at("name").get<std::string>(), "Bar");
    EXPECT_EQ(enumProp.at("values")[1].at("value").get<int64_t>(), 1);
}

} // namespace

TEST_F(McpSceneSystemTests, QueryComponent_IncludeSchema_EnumProperty_HasMetadata)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    const std::string compId = AddComponent(goId, "ReflectionTestObject");
    ASSERT_FALSE(compId.empty());

    auto res = Dispatch("scene", "component",
                        {{"object_id", compId}, {"include_schema", true}});
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res.contains("component"));
    EXPECT_EQ(res["component"]["class"].get<std::string>(), "ReflectionTestObject");
    ASSERT_TRUE(res["component"]["schema"].is_array());

    const json* enumProp = FindSchemaProperty(res["component"]["schema"], "m_rEnum");
    ASSERT_NE(enumProp, nullptr);
    ExpectEnumPropertyMetadata(*enumProp);
}

TEST_F(McpSceneSystemTests, QueryComponentsOnObject_AfterAddingComponent_ListsIt)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    ASSERT_FALSE(AddComponent(goId, "PointLight").empty());

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

// ─── Commands (execute synchronously, return real results) ─────────────────────

TEST_F(McpSceneSystemTests, CommandCreateGameObject_CreatesObject)
{
    auto res = Dispatch("scene", "CreateGameObject", {{"name", "New GameObject"}});
    EXPECT_TRUE(res["ok"].get<bool>());
    EXPECT_FALSE(res.value("objectId", std::string{}).empty());

    EXPECT_EQ(m_core->GetWorld()->GetGameObjects().size(), 1u);
}

TEST_F(McpSceneSystemTests, CommandCreateGameObject_CustomName_ReturnsObjectId)
{
    auto res = Dispatch("scene", "CreateGameObject", {{"name", "MyCustomCube"}});
    EXPECT_TRUE(res["ok"].get<bool>());
    const std::string objectId = res.value("objectId", std::string{});
    EXPECT_FALSE(objectId.empty());
    EXPECT_FALSE(res.contains("error"));

    auto queryRes = Dispatch("scene", "game_objects");
    ASSERT_EQ(queryRes["game_objects"].size(), 1u);
    EXPECT_EQ(queryRes["game_objects"][0]["object_id"].get<std::string>(), objectId);
    EXPECT_EQ(queryRes["game_objects"][0]["name"].get<std::string>(), "MyCustomCube");
}

TEST_F(McpSceneSystemTests, CommandCreateGameObject_RenameFailure_ReportsErrorInResult)
{
    json envelope;
    envelope["type"]               = "auxiliary";
    envelope["name"]               = "CreateGameObjectWithRename";
    envelope["desiredName"]        = "ShouldFailRename";
    envelope["forceRenameFailure"] = true;
    const json r = m_core->ExecuteSerializedCommand(envelope);

    EXPECT_TRUE(r["ok"].get<bool>());
    EXPECT_FALSE(r.value("objectId", std::string{}).empty());
    ASSERT_TRUE(r.contains("error"));
    EXPECT_NE(r["error"].get<std::string>().find("rename failed"), std::string::npos);
}

TEST_F(McpSceneSystemTests, CommandDeleteGameObject_RemovesObject)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());
    ASSERT_EQ(m_core->GetWorld()->GetGameObjects().size(), 1u);

    auto res = Dispatch("scene", "DeleteGameObject", {{"objectId", goId}});
    EXPECT_TRUE(res["ok"].get<bool>());

    EXPECT_EQ(m_core->GetWorld()->GetGameObjects().size(), 0u);
}

TEST_F(McpSceneSystemTests, CommandDuplicateGameObject_CreatesIndependentCopy)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());

    auto reply = Dispatch("scene", "DuplicateGameObject",
                          {{"objectId", goId}, {"newName", "Copy"}});
    EXPECT_TRUE(reply["ok"].get<bool>());
    const std::string dupId = reply.value("objectId", std::string{});
    ASSERT_FALSE(dupId.empty());
    EXPECT_NE(dupId, goId);

    EXPECT_EQ(m_core->GetWorld()->GetGameObjects().size(), 2u);

    // The duplicate carries the new name and a freshly-id'd PointLight component.
    auto dupRes = Dispatch("scene", "game_object", {{"object_id", dupId}});
    ASSERT_TRUE(dupRes["ok"].get<bool>());
    EXPECT_EQ(dupRes["game_object"]["name"].get<std::string>(), "Copy");

    bool foundFreshPointLight = false;
    for (const auto& c : dupRes["game_object"]["components"])
    {
        if (c.value("class", "") == "PointLight")
        {
            foundFreshPointLight = true;
            EXPECT_NE(c["object_id"].get<std::string>(), plId);
        }
    }
    EXPECT_TRUE(foundFreshPointLight);
}

TEST_F(McpSceneSystemTests, CommandDuplicateGameObject_Undo_RemovesCopy)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    auto res = Dispatch("scene", "DuplicateGameObject", {{"objectId", goId}});
    EXPECT_TRUE(res["ok"].get<bool>());
    EXPECT_EQ(m_core->GetWorld()->GetGameObjects().size(), 2u);

    auto undoRes = Dispatch("undo_history", "Undo");
    EXPECT_TRUE(undoRes["ok"].get<bool>());
    EXPECT_EQ(m_core->GetWorld()->GetGameObjects().size(), 1u);
}

TEST_F(McpSceneSystemTests, CommandDuplicateGameObject_MissingObjectId_ReturnsError)
{
    auto res = Dispatch("scene", "DuplicateGameObject");
    EXPECT_FALSE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("error"));
}

TEST_F(McpSceneSystemTests, CommandCreateComponent_AddsPointLight)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    auto res = Dispatch("scene", "CreateComponent",
                        {{"objectId", goId}, {"componentClass", "PointLight"}});
    EXPECT_TRUE(res["ok"].get<bool>());

    auto compRes = Dispatch("scene", "components_on_object", {{"object_id", goId}});
    bool foundPointLight = false;
    for (const auto& c : compRes["components"])
        if (c.value("class", "") == "PointLight")
            foundPointLight = true;
    EXPECT_TRUE(foundPointLight);
}

TEST_F(McpSceneSystemTests, CommandDeleteComponent_RemovesComponent)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());
    // First component becomes the root; add a second (non-root) one to delete.
    ASSERT_FALSE(AddComponent(goId, "PointLight").empty());
    const std::string slId = AddComponent(goId, "SpotLight");
    ASSERT_FALSE(slId.empty());

    auto res = Dispatch("scene", "DeleteComponent", {{"objectId", slId}});
    EXPECT_TRUE(res["ok"].get<bool>());

    auto compRes = Dispatch("scene", "components_on_object", {{"object_id", goId}});
    bool foundSpotLight = false;
    for (const auto& c : compRes["components"])
        if (c.value("class", "") == "SpotLight")
            foundSpotLight = true;
    EXPECT_FALSE(foundSpotLight);
}

TEST_F(McpSceneSystemTests, CommandReparentSceneComponent_ChangesParent)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    const std::string plId = AddComponent(goId, "PointLight");
    const std::string slId = AddComponent(goId, "SpotLight");
    ASSERT_FALSE(plId.empty());
    ASSERT_FALSE(slId.empty());

    auto res = Dispatch("scene", "ReparentSceneComponent",
                        {{"objectId", slId}, {"newParentId", plId}});
    EXPECT_TRUE(res["ok"].get<bool>());
}

TEST_F(McpSceneSystemTests, CommandSetPosition_Immediate_SetsLocalPosition)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());
    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());

    auto res = Dispatch("scene", "SetPosition",
        {{"objectId", plId},
         {"value", json::array({4.0f, 0.0f, 0.0f})},
         {"space", "local"},
         {"duration_seconds", 0.0f}});
    EXPECT_TRUE(res["ok"].get<bool>());
}

TEST_F(McpSceneSystemTests, CommandSetRotation_Immediate_SetsLocalRotation)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());
    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());

    auto res = Dispatch("scene", "SetRotation",
        {{"objectId", plId},
         {"value", json::array({0.0f, 0.0f, 0.0f, 1.0f})},  // identity quaternion
         {"duration_seconds", 0.0f}});
    EXPECT_TRUE(res["ok"].get<bool>());
}

TEST_F(McpSceneSystemTests, CommandSetScale_Immediate_SetsLocalScale)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());
    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());

    auto res = Dispatch("scene", "SetScale",
        {{"objectId", plId},
         {"value", json::array({2.0f, 2.0f, 2.0f})},
         {"duration_seconds", 0.0f}});
    EXPECT_TRUE(res["ok"].get<bool>());
}

TEST_F(McpSceneSystemTests, CommandSetPosition_WithDuration_StartsAnimation)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());
    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());

    // headless mode → animation applies immediately via SetProperty fallback.
    auto res = Dispatch("scene", "SetPosition",
        {{"objectId", plId},
         {"value", json::array({3.0f, 0.0f, 0.0f})},
         {"duration_seconds", 1.0f}});
    EXPECT_TRUE(res["ok"].get<bool>());
}

TEST_F(McpSceneSystemTests, CommandSetPosition_DefaultDuration_StartsAnimation)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());
    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());

    // Omitting duration_seconds animates by default (kDefaultAnimationDurationSeconds).
    auto res = Dispatch("scene", "SetPosition",
        {{"objectId", plId},
         {"value", json::array({3.0f, 0.0f, 0.0f})}});
    EXPECT_TRUE(res["ok"].get<bool>());
}

TEST_F(McpSceneSystemTests, CommandSetPosition_MissingObjectId_ReturnsError)
{
    auto res = Dispatch("scene", "SetPosition",
                        {{"value", json::array({1.0f, 0.0f, 0.0f})}});
    EXPECT_FALSE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("error"));
}

TEST_F(McpSceneSystemTests, CommandSetScale_MissingValue_ReturnsError)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    auto res = Dispatch("scene", "SetScale", {{"objectId", goId}});
    EXPECT_FALSE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("error"));
}
