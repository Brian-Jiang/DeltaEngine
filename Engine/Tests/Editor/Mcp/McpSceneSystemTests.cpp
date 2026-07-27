#include "Editor/Mcp/McpCoreFixture.h"

#include "Editor/Mcp/McpQueryRouter.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <string>
#include <utility>

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class McpSceneSystemTests : public McpCoreFixture
{
protected:
    // Adds a component to a GameObject through the command manager; returns its objectId.
    std::string AddComponent(const std::string& goId, const char* className)
    {
        return ExecCreateComponent(goId, className);
    }

    SceneComponent* GetSceneComponent(const std::string& objectId) const
    {
        return m_core->ResolveObject<SceneComponent>(
            GetActiveSceneAssetId(), DeltaEngine::UUID::FromString(objectId));
    }

    bool Undo() const
    {
        EditorCommandContext ctx{ *m_core };
        return m_core->GetCommandManager().Undo(ctx);
    }

    size_t UndoDepth() const { return m_core->GetCommandManager().GetUndoStackDepth(); }

    static void ExpectVec3Near(const DirectX::SimpleMath::Vector3& actual,
                               float x, float y, float z, float tolerance = 1e-4f)
    {
        EXPECT_NEAR(actual.x, x, tolerance);
        EXPECT_NEAR(actual.y, y, tolerance);
        EXPECT_NEAR(actual.z, z, tolerance);
    }

    // Compares rotations by the direction they send +Z, so the q / -q double cover doesn't matter.
    static void ExpectForwardNear(const DirectX::SimpleMath::Quaternion& rotation,
                                  float x, float y, float z, float tolerance = 1e-4f)
    {
        const DirectX::SimpleMath::Vector3 forward =
            DirectX::SimpleMath::Vector3::Transform(
                DirectX::SimpleMath::Vector3::UnitZ, rotation);
        ExpectVec3Near(forward, x, y, z, tolerance);
    }

    // Root scene component, plus a child reparented under it. Returns { rootId, childId }.
    std::pair<std::string, std::string> MakeParentChild()
    {
        const std::string goId = ExecCreateGameObject();
        EXPECT_FALSE(goId.empty());
        const std::string rootId = AddComponent(goId, "PointLight");
        const std::string childId = AddComponent(goId, "SpotLight");
        EXPECT_FALSE(rootId.empty());
        EXPECT_FALSE(childId.empty());

        auto reparent = Dispatch("scene", "ReparentSceneComponent",
                                 {{"objectId", childId}, {"newParentId", rootId}});
        EXPECT_TRUE(reparent["ok"].get<bool>());
        return { rootId, childId };
    }

    void SetPositionImmediate(const std::string& objectId, float x, float y, float z,
                              const char* space = "local")
    {
        auto res = Dispatch("scene", "SetPosition",
            {{"objectId", objectId},
             {"value", json::array({x, y, z})},
             {"space", space},
             {"duration_seconds", 0.0f}});
        EXPECT_TRUE(res["ok"].get<bool>());
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

// ─── Transform commands ──────────────────────────────────────────────────────

TEST_F(McpSceneSystemTests, CommandSetPosition_Immediate_SetsLocalPosition)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());
    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());

    auto res = Dispatch("scene", "SetPosition",
        {{"objectId", plId},
         {"value", json::array({4.0f, -1.0f, 2.5f})},
         {"space", "local"},
         {"duration_seconds", 0.0f}});
    ASSERT_TRUE(res["ok"].get<bool>());

    SceneComponent* sc = GetSceneComponent(plId);
    ASSERT_NE(sc, nullptr);
    ExpectVec3Near(sc->GetLocalPosition(), 4.0f, -1.0f, 2.5f);
}

TEST_F(McpSceneSystemTests, CommandSetPosition_Immediate_WorldSpace_ConvertsThroughParent)
{
    auto [rootId, childId] = MakeParentChild();
    SetPositionImmediate(rootId, 10.0f, 0.0f, 0.0f);

    auto res = Dispatch("scene", "SetPosition",
        {{"objectId", childId},
         {"value", json::array({12.0f, 3.0f, 0.0f})},
         {"space", "world"},
         {"duration_seconds", 0.0f}});
    ASSERT_TRUE(res["ok"].get<bool>());

    SceneComponent* child = GetSceneComponent(childId);
    ASSERT_NE(child, nullptr);
    ExpectVec3Near(child->GetLocalPosition(), 2.0f, 3.0f, 0.0f);
    ExpectVec3Near(child->GetWorldPosition(), 12.0f, 3.0f, 0.0f);
}

TEST_F(McpSceneSystemTests, CommandSetPosition_Immediate_Undo_RestoresPreviousPosition)
{
    const std::string goId = CreateLegacyGameObject();
    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());

    SetPositionImmediate(plId, 1.0f, 2.0f, 3.0f);
    SetPositionImmediate(plId, 7.0f, 8.0f, 9.0f);

    SceneComponent* sc = GetSceneComponent(plId);
    ASSERT_NE(sc, nullptr);
    ExpectVec3Near(sc->GetLocalPosition(), 7.0f, 8.0f, 9.0f);

    ASSERT_TRUE(Undo());
    ExpectVec3Near(sc->GetLocalPosition(), 1.0f, 2.0f, 3.0f);
}

TEST_F(McpSceneSystemTests, CommandSetRotation_Immediate_Quaternion_SetsRotationAndSyncsEuler)
{
    const std::string goId = CreateLegacyGameObject();
    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());

    // 90 degree yaw as a quaternion.
    constexpr float kHalfRoot2 = 0.70710678f;
    auto res = Dispatch("scene", "SetRotation",
        {{"objectId", plId},
         {"value", json::array({0.0f, kHalfRoot2, 0.0f, kHalfRoot2})},
         {"duration_seconds", 0.0f}});
    ASSERT_TRUE(res["ok"].get<bool>());

    SceneComponent* sc = GetSceneComponent(plId);
    ASSERT_NE(sc, nullptr);
    ExpectForwardNear(sc->GetLocalRotation(), 1.0f, 0.0f, 0.0f, 1e-3f);
    // The euler hint must be cross-synced from the quaternion by PostEditChangeProperty.
    ExpectVec3Near(sc->GetLocalRotationEulerAngles(), 0.0f, 90.0f, 0.0f, 1e-2f);
}

TEST_F(McpSceneSystemTests, CommandSetRotation_Immediate_EulerDegrees_PreservedVerbatim)
{
    const std::string goId = CreateLegacyGameObject();
    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());

    // 370 degrees must survive as 370, not be normalised to 10.
    auto res = Dispatch("scene", "SetRotation",
        {{"objectId", plId},
         {"value", json::array({0.0f, 370.0f, 0.0f})},
         {"duration_seconds", 0.0f}});
    ASSERT_TRUE(res["ok"].get<bool>());

    SceneComponent* sc = GetSceneComponent(plId);
    ASSERT_NE(sc, nullptr);
    ExpectVec3Near(sc->GetLocalRotationEulerAngles(), 0.0f, 370.0f, 0.0f, 1e-3f);
    // ...while the quaternion is the equivalent 10 degree yaw.
    constexpr float kSin10 = 0.17364818f;
    constexpr float kCos10 = 0.98480775f;
    ExpectForwardNear(sc->GetLocalRotation(), kSin10, 0.0f, kCos10, 1e-3f);
}

TEST_F(McpSceneSystemTests, CommandSetRotation_Immediate_WorldSpace_ConvertsThroughParent)
{
    auto [rootId, childId] = MakeParentChild();

    auto rootRes = Dispatch("scene", "SetRotation",
        {{"objectId", rootId},
         {"value", json::array({0.0f, 90.0f, 0.0f})},
         {"duration_seconds", 0.0f}});
    ASSERT_TRUE(rootRes["ok"].get<bool>());

    // Ask for the same world rotation the parent already has → child local becomes identity.
    auto res = Dispatch("scene", "SetRotation",
        {{"objectId", childId},
         {"value", json::array({0.0f, 90.0f, 0.0f})},
         {"space", "world"},
         {"duration_seconds", 0.0f}});
    ASSERT_TRUE(res["ok"].get<bool>());

    SceneComponent* child = GetSceneComponent(childId);
    ASSERT_NE(child, nullptr);
    ExpectForwardNear(child->GetLocalRotation(), 0.0f, 0.0f, 1.0f, 1e-3f);
    ExpectForwardNear(child->GetWorldRotation(), 1.0f, 0.0f, 0.0f, 1e-3f);
}

TEST_F(McpSceneSystemTests, CommandSetScale_Immediate_SetsLocalScale)
{
    const std::string goId = CreateLegacyGameObject();
    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());

    auto res = Dispatch("scene", "SetScale",
        {{"objectId", plId},
         {"value", json::array({2.0f, 3.0f, 4.0f})},
         {"duration_seconds", 0.0f}});
    ASSERT_TRUE(res["ok"].get<bool>());

    SceneComponent* sc = GetSceneComponent(plId);
    ASSERT_NE(sc, nullptr);
    ExpectVec3Near(sc->GetLocalScale(), 2.0f, 3.0f, 4.0f);
}

TEST_F(McpSceneSystemTests, CommandSetScale_WorldSpace_ReturnsError)
{
    const std::string goId = CreateLegacyGameObject();
    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());

    auto res = Dispatch("scene", "SetScale",
        {{"objectId", plId},
         {"value", json::array({2.0f, 2.0f, 2.0f})},
         {"space", "world"},
         {"duration_seconds", 0.0f}});
    EXPECT_FALSE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("error"));
}

TEST_F(McpSceneSystemTests, CommandSetPosition_WithDuration_HeadlessAppliesImmediately)
{
    const std::string goId = CreateLegacyGameObject();
    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());

    const size_t depthBefore = UndoDepth();

    // Headless has no animation manager → the fallback applies the value and commits now.
    auto res = Dispatch("scene", "SetPosition",
        {{"objectId", plId},
         {"value", json::array({3.0f, 0.0f, 0.0f})},
         {"duration_seconds", 1.0f}});
    ASSERT_TRUE(res["ok"].get<bool>());

    SceneComponent* sc = GetSceneComponent(plId);
    ASSERT_NE(sc, nullptr);
    ExpectVec3Near(sc->GetLocalPosition(), 3.0f, 0.0f, 0.0f);
    EXPECT_EQ(UndoDepth(), depthBefore + 1);
}

TEST_F(McpSceneSystemTests, CommandSetPosition_DefaultDuration_HeadlessAppliesImmediately)
{
    const std::string goId = CreateLegacyGameObject();
    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());

    // Omitting duration_seconds animates by default (kDefaultAnimationDurationSeconds).
    auto res = Dispatch("scene", "SetPosition",
        {{"objectId", plId},
         {"value", json::array({3.0f, 0.0f, 0.0f})}});
    ASSERT_TRUE(res["ok"].get<bool>());

    SceneComponent* sc = GetSceneComponent(plId);
    ASSERT_NE(sc, nullptr);
    ExpectVec3Near(sc->GetLocalPosition(), 3.0f, 0.0f, 0.0f);
}

TEST_F(McpSceneSystemTests, CommandSetRotation_WithDuration_EulerDegrees_HeadlessApplies)
{
    const std::string goId = CreateLegacyGameObject();
    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());

    auto res = Dispatch("scene", "SetRotation",
        {{"objectId", plId},
         {"value", json::array({0.0f, 90.0f, 0.0f})},
         {"duration_seconds", 1.0f}});
    ASSERT_TRUE(res["ok"].get<bool>());

    SceneComponent* sc = GetSceneComponent(plId);
    ASSERT_NE(sc, nullptr);
    ExpectForwardNear(sc->GetLocalRotation(), 1.0f, 0.0f, 0.0f, 1e-3f);
}

TEST_F(McpSceneSystemTests, CommandSetScale_WithDuration_HeadlessApplies)
{
    const std::string goId = CreateLegacyGameObject();
    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());

    auto res = Dispatch("scene", "SetScale",
        {{"objectId", plId},
         {"value", json::array({5.0f, 5.0f, 5.0f})},
         {"duration_seconds", 1.0f}});
    ASSERT_TRUE(res["ok"].get<bool>());

    SceneComponent* sc = GetSceneComponent(plId);
    ASSERT_NE(sc, nullptr);
    ExpectVec3Near(sc->GetLocalScale(), 5.0f, 5.0f, 5.0f);
}

TEST_F(McpSceneSystemTests, CommandSetPosition_MissingObjectId_ReturnsError)
{
    auto res = Dispatch("scene", "SetPosition",
                        {{"value", json::array({1.0f, 0.0f, 0.0f})}});
    EXPECT_FALSE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("error"));
}

TEST_F(McpSceneSystemTests, CommandSetPosition_InvalidSpace_ReturnsError)
{
    const std::string goId = CreateLegacyGameObject();
    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());

    auto res = Dispatch("scene", "SetPosition",
        {{"objectId", plId},
         {"value", json::array({1.0f, 0.0f, 0.0f})},
         {"space", "galactic"},
         {"duration_seconds", 0.0f}});
    EXPECT_FALSE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("error"));
}

TEST_F(McpSceneSystemTests, CommandSetRotation_WrongElementCount_ReturnsError)
{
    const std::string goId = CreateLegacyGameObject();
    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());

    auto res = Dispatch("scene", "SetRotation",
        {{"objectId", plId},
         {"value", json::array({1.0f, 0.0f})},
         {"duration_seconds", 0.0f}});
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

// ─── Transform queries ───────────────────────────────────────────────────────

TEST_F(McpSceneSystemTests, QueryGetPosition_ReturnsLocalAndWorld)
{
    auto [rootId, childId] = MakeParentChild();
    SetPositionImmediate(rootId, 10.0f, 0.0f, 0.0f);
    SetPositionImmediate(childId, 2.0f, 3.0f, 0.0f);

    auto local = Dispatch("scene", "get_position", {{"object_id", childId}});
    ASSERT_TRUE(local["ok"].get<bool>());
    EXPECT_EQ(local["space"].get<std::string>(), "local");
    EXPECT_EQ(local["object_id"].get<std::string>(), childId);
    ASSERT_TRUE(local["position"].is_array());
    EXPECT_NEAR(local["position"][0].get<float>(), 2.0f, 1e-4f);
    EXPECT_NEAR(local["position"][1].get<float>(), 3.0f, 1e-4f);

    auto world = Dispatch("scene", "get_position",
                          {{"object_id", childId}, {"space", "world"}});
    ASSERT_TRUE(world["ok"].get<bool>());
    EXPECT_EQ(world["space"].get<std::string>(), "world");
    EXPECT_NEAR(world["position"][0].get<float>(), 12.0f, 1e-4f);
    EXPECT_NEAR(world["position"][1].get<float>(), 3.0f, 1e-4f);
}

TEST_F(McpSceneSystemTests, QueryGetPosition_GameObjectId_ResolvesRootComponent)
{
    const std::string goId = CreateLegacyGameObject();
    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());
    SetPositionImmediate(plId, 1.0f, 2.0f, 3.0f);

    auto res = Dispatch("scene", "get_position", {{"object_id", goId}});
    ASSERT_TRUE(res["ok"].get<bool>());
    EXPECT_EQ(res["object_id"].get<std::string>(), plId);
    EXPECT_NEAR(res["position"][0].get<float>(), 1.0f, 1e-4f);
    EXPECT_NEAR(res["position"][2].get<float>(), 3.0f, 1e-4f);
}

TEST_F(McpSceneSystemTests, QueryGetRotation_ReturnsQuaternionAndEulerDegrees)
{
    const std::string goId = CreateLegacyGameObject();
    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());

    auto set = Dispatch("scene", "SetRotation",
        {{"objectId", plId},
         {"value", json::array({0.0f, 90.0f, 0.0f})},
         {"duration_seconds", 0.0f}});
    ASSERT_TRUE(set["ok"].get<bool>());

    auto res = Dispatch("scene", "get_rotation", {{"object_id", plId}});
    ASSERT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res["quaternion"].is_array());
    ASSERT_EQ(res["quaternion"].size(), 4u);
    ASSERT_TRUE(res["euler"].is_array());
    ASSERT_EQ(res["euler"].size(), 3u);

    // Degrees, echoed back exactly as written.
    EXPECT_NEAR(res["euler"][0].get<float>(), 0.0f, 1e-3f);
    EXPECT_NEAR(res["euler"][1].get<float>(), 90.0f, 1e-3f);
    EXPECT_NEAR(res["euler"][2].get<float>(), 0.0f, 1e-3f);

    constexpr float kHalfRoot2 = 0.70710678f;
    EXPECT_NEAR(std::abs(res["quaternion"][1].get<float>()), kHalfRoot2, 1e-3f);
    EXPECT_NEAR(std::abs(res["quaternion"][3].get<float>()), kHalfRoot2, 1e-3f);
}

TEST_F(McpSceneSystemTests, QueryGetRotation_WorldSpace_CombinesParentRotation)
{
    auto [rootId, childId] = MakeParentChild();

    auto set = Dispatch("scene", "SetRotation",
        {{"objectId", rootId},
         {"value", json::array({0.0f, 90.0f, 0.0f})},
         {"duration_seconds", 0.0f}});
    ASSERT_TRUE(set["ok"].get<bool>());

    auto res = Dispatch("scene", "get_rotation",
                        {{"object_id", childId}, {"space", "world"}});
    ASSERT_TRUE(res["ok"].get<bool>());
    EXPECT_EQ(res["space"].get<std::string>(), "world");
    EXPECT_NEAR(res["euler"][1].get<float>(), 90.0f, 1e-2f);
}

TEST_F(McpSceneSystemTests, QueryGetScale_ReturnsLocalAndWorld)
{
    auto [rootId, childId] = MakeParentChild();

    auto setRoot = Dispatch("scene", "SetScale",
        {{"objectId", rootId}, {"value", json::array({2.0f, 2.0f, 2.0f})},
         {"duration_seconds", 0.0f}});
    ASSERT_TRUE(setRoot["ok"].get<bool>());
    auto setChild = Dispatch("scene", "SetScale",
        {{"objectId", childId}, {"value", json::array({3.0f, 3.0f, 3.0f})},
         {"duration_seconds", 0.0f}});
    ASSERT_TRUE(setChild["ok"].get<bool>());

    auto local = Dispatch("scene", "get_scale", {{"object_id", childId}});
    ASSERT_TRUE(local["ok"].get<bool>());
    EXPECT_NEAR(local["scale"][0].get<float>(), 3.0f, 1e-4f);

    auto world = Dispatch("scene", "get_scale",
                          {{"object_id", childId}, {"space", "world"}});
    ASSERT_TRUE(world["ok"].get<bool>());
    EXPECT_NEAR(world["scale"][0].get<float>(), 6.0f, 1e-3f);
}

TEST_F(McpSceneSystemTests, QueryGetPosition_MissingObjectId_ReturnsError)
{
    auto res = Dispatch("scene", "get_position");
    EXPECT_FALSE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("error"));
}

TEST_F(McpSceneSystemTests, QueryGetPosition_InvalidSpace_ReturnsError)
{
    const std::string goId = CreateLegacyGameObject();
    const std::string plId = AddComponent(goId, "PointLight");
    ASSERT_FALSE(plId.empty());

    auto res = Dispatch("scene", "get_position",
                        {{"object_id", plId}, {"space", "galactic"}});
    EXPECT_FALSE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("error"));
}

TEST_F(McpSceneSystemTests, QueryGetScale_UnknownObject_ReturnsError)
{
    auto res = Dispatch("scene", "get_scale",
                        {{"object_id", "00000000-0000-0000-0000-000000000000"}});
    EXPECT_FALSE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("error"));
}
