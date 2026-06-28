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

TEST_F(EditorCoreTests, EnqueueSerializedCommand_EmptyPayload_DroppedSilently)
{
    m_core->EnqueueSerializedCommand("");

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);

    EXPECT_TRUE(responses.empty());
}

TEST_F(EditorCoreTests, DrainCommandQueue_JsonParseError_ReturnsErrorEnvelope)
{
    m_core->EnqueueSerializedCommand("not json {{");

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);

    ASSERT_EQ(responses.size(), 1u);
    const auto j = nlohmann::json::parse(responses[0]);
    EXPECT_FALSE(j.value("ok", true));
    EXPECT_NE(j.value("error", std::string{}).find("JSON parse error"), std::string::npos);
}

TEST_F(EditorCoreTests, DrainCommandQueue_MissingType_ReturnsErrorEnvelope)
{
    m_core->EnqueueSerializedCommand(R"({"data":{}})");

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);

    ASSERT_EQ(responses.size(), 1u);
    const auto j = nlohmann::json::parse(responses[0]);
    EXPECT_FALSE(j.value("ok", true));
    EXPECT_NE(j.value("error", std::string{}).find("type"), std::string::npos);
}

TEST_F(EditorCoreTests, DrainCommandQueue_UnknownCommand_ReturnsErrorEnvelope)
{
    m_core->EnqueueSerializedCommand(R"({"type":"EditorCommand_NoSuchThing","data":{}})");

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);

    ASSERT_EQ(responses.size(), 1u);
    const auto j = nlohmann::json::parse(responses[0]);
    EXPECT_FALSE(j.value("ok", true));
    EXPECT_EQ(j.value("commandType", std::string{}), "EditorCommand_NoSuchThing");
}

TEST_F(EditorCoreTests, DrainCommandQueue_WithEmbeddedRequestId_WrapsResult)
{
    const AssetId sceneId = GetActiveSceneAssetId();
    nlohmann::json data;
    data["sceneAssetId"] = sceneId.ToString();
    data["className"] = "GameObject";
    nlohmann::json envelope;
    envelope["type"] = "EditorCommand_CreateGameObject";
    envelope["data"] = data;
    envelope["request_id"] = "req-direct-1";

    m_core->EnqueueSerializedCommand(envelope.dump());

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);

    ASSERT_EQ(responses.size(), 1u);
    const auto j = nlohmann::json::parse(responses[0]);
    EXPECT_EQ(j["phase"].get<std::string>(), "result");
    EXPECT_EQ(j["request_id"].get<std::string>(), "req-direct-1");
    EXPECT_TRUE(j["ok"].get<bool>());
    EXPECT_EQ(j["commandType"].get<std::string>(), "EditorCommand_CreateGameObject");
    EXPECT_FALSE(j["objectId"].get<std::string>().empty());
}

TEST_F(EditorCoreTests, EnqueueSerializedCommand_ActiveRequestId_EmbedsInEnvelope)
{
    m_core->SetActiveMcpRequestId("req-active-2");

    nlohmann::json envelope;
    envelope["type"] = "auxiliary";
    envelope["name"] = "SaveDirtyAssets";
    m_core->EnqueueSerializedCommand(envelope.dump());

    m_core->ClearActiveMcpRequestId();

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);

    ASSERT_EQ(responses.size(), 1u);
    const auto j = nlohmann::json::parse(responses[0]);
    EXPECT_EQ(j["phase"].get<std::string>(), "result");
    EXPECT_EQ(j["request_id"].get<std::string>(), "req-active-2");
    EXPECT_TRUE(j["ok"].get<bool>());
    EXPECT_EQ(j["commandType"].get<std::string>(), "SaveDirtyAssets");
}

TEST_F(EditorCoreTests, LoadScene_NonExistentPath_DoesNothing)
{
    const auto missing = m_tempDir / "missing_scene.dasset.json";
    m_core->LoadScene(missing);

    EXPECT_NE(m_core->GetActiveSceneAsset(), nullptr);
}
