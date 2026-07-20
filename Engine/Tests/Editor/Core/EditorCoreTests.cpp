#include "../EditorCoreFixture.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Runtime/Assets/DPrimaryAsset.h"

#include <nlohmann/json.hpp>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

namespace
{
class EditorCoreTests : public EditorCoreFixture
{
};
}

TEST_F(EditorCoreTests, ResolveObject_NullAssetId_ReturnsNull)
{
    EXPECT_EQ(m_core->ResolveObject(AssetId::Null(), ObjectId::Generate()), nullptr);
}

TEST_F(EditorCoreTests, ResolveObject_NullObjectId_ReturnsNull)
{
    EXPECT_EQ(m_core->ResolveObject(GetActiveSceneAssetId(), ObjectId::Null()), nullptr);
}

TEST_F(EditorCoreTests, ResolveObject_UnregisteredAsset_ReturnsNull)
{
    EXPECT_EQ(m_core->ResolveObject(AssetId::Generate(), ObjectId::Generate()), nullptr);
}

TEST_F(EditorCoreTests, ResolveObject_UnknownObjectInLoadedAsset_ReturnsNull)
{
    const AssetId sceneId = GetActiveSceneAssetId();
    ASSERT_FALSE(sceneId.IsNull());
    EXPECT_EQ(m_core->ResolveObject(sceneId, ObjectId::Generate()), nullptr);
}

TEST_F(EditorCoreTests, GetIdsForObject_Null_ReturnsNullPair)
{
    const auto [aid, oid] = m_core->GetIdsForObject(nullptr);
    EXPECT_TRUE(aid.IsNull());
    EXPECT_TRUE(oid.IsNull());
}

TEST_F(EditorCoreTests, GetIdsForObject_RoundTripsLoadedSceneAsset)
{
    DPrimaryAsset* scene = m_core->GetActiveSceneAsset();
    ASSERT_NE(scene, nullptr);

    const auto& objects = scene->GetObjects();
    if (objects.empty())
        GTEST_SKIP() << "Default scene has no objects to round-trip";

    DObject* first = objects.front();
    const auto [aid, oid] = m_core->GetIdsForObject(first);
    EXPECT_EQ(aid, scene->GetAssetId());
    EXPECT_EQ(oid, first->GetObjectId());
}

TEST_F(EditorCoreTests, ExecuteSerializedCommand_MissingType_ReturnsError)
{
    const auto j = m_core->ExecuteSerializedCommand(nlohmann::json::parse(R"({"data":{}})"));
    EXPECT_FALSE(j.value("ok", true));
    EXPECT_NE(j.value("error", std::string{}).find("type"), std::string::npos);
}

TEST_F(EditorCoreTests, ExecuteSerializedCommand_UnknownCommand_ReturnsError)
{
    const auto j = m_core->ExecuteSerializedCommand(
        nlohmann::json::parse(R"({"type":"EditorCommand_NoSuchThing","data":{}})"));
    EXPECT_FALSE(j.value("ok", true));
    EXPECT_EQ(j.value("commandType", std::string{}), "EditorCommand_NoSuchThing");
}

TEST_F(EditorCoreTests, ExecuteSerializedCommand_CreateGameObject_ReturnsResult)
{
    const AssetId sceneId = GetActiveSceneAssetId();
    nlohmann::json data;
    data["sceneAssetId"] = sceneId.ToString();
    data["className"] = "GameObject";
    nlohmann::json envelope;
    envelope["type"] = "EditorCommand_CreateGameObject";
    envelope["data"] = data;

    const auto j = m_core->ExecuteSerializedCommand(envelope);
    EXPECT_TRUE(j["ok"].get<bool>());
    EXPECT_EQ(j["commandType"].get<std::string>(), "EditorCommand_CreateGameObject");
    EXPECT_FALSE(j["objectId"].get<std::string>().empty());
}

TEST_F(EditorCoreTests, ExecuteSerializedCommand_SaveDirtyAssetsAuxiliary_Succeeds)
{
    nlohmann::json envelope;
    envelope["type"] = "auxiliary";
    envelope["name"] = "SaveDirtyAssets";

    const auto j = m_core->ExecuteSerializedCommand(envelope);
    EXPECT_TRUE(j["ok"].get<bool>());
    EXPECT_EQ(j["commandType"].get<std::string>(), "SaveDirtyAssets");
}

TEST_F(EditorCoreTests, LoadScene_NonExistentPath_DoesNothing)
{
    const auto missing = m_tempDir / "missing_scene.dasset.json";
    m_core->LoadScene(missing);

    EXPECT_NE(m_core->GetActiveSceneAsset(), nullptr);
}
