#include "Editor/Mcp/McpCoreFixture.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/Commands/PropertyValueIO.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Test/SerializationTestTypes.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class McpCommonSystemTests : public McpCoreFixture {};

TEST_F(McpCommonSystemTests, CommandRenameObject_ChangesName)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    auto dispatchRes = Dispatch("common", "RenameObject",
                                {{"objectId", goId}, {"newName", "RenamedGO"}});
    EXPECT_TRUE(dispatchRes["ok"].get<bool>());
    EXPECT_TRUE(dispatchRes.value("queued", false));

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_TRUE(json::parse(responses[0])["ok"].get<bool>());

    // Verify name changed
    auto goRes = Dispatch("scene", "game_object", {{"object_id", goId}});
    EXPECT_EQ(goRes["game_object"]["name"].get<std::string>(), "RenamedGO");
}

TEST_F(McpCommonSystemTests, CommandSetProperty_FloatProperty_ChangesValue)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    // Add PointLight via legacy path
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
        ASSERT_TRUE(json::parse(r[0])["ok"].get<bool>());
    }

    // Find PointLight's object_id
    auto comps = Dispatch("scene", "components_on_object", {{"object_id", goId}});
    std::string plId;
    for (const auto& c : comps["components"])
        if (c.value("class", "") == "PointLight")
            plId = c["object_id"].get<std::string>();
    ASSERT_FALSE(plId.empty());

    auto dispatchRes = Dispatch("common", "SetProperty",
                                {{"objectId",     plId},
                                 {"propertyName", "m_intensity"},
                                 {"valueAfter",   4.5f}});
    EXPECT_TRUE(dispatchRes["ok"].get<bool>());

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_TRUE(json::parse(responses[0])["ok"].get<bool>());

    // Verify via component query
    auto compRes = Dispatch("scene", "component", {{"object_id", plId}});
    EXPECT_TRUE(compRes["ok"].get<bool>());
    const float intensity = compRes["component"]["properties"]["m_intensity"].get<float>();
    EXPECT_NEAR(intensity, 4.5f, 0.001f);
}

TEST_F(McpCommonSystemTests, CommandRenameObject_WithExplicitAssetId_RenamesObjectInOtherAsset)
{
    auto* asset = CreateDObject<PA_TestAsset>();
    const AssetId otherAssetId = AssetId::Generate();
    asset->GetHeader().m_persistentId = otherAssetId;
    asset->GetHeader().m_className = "PA_TestAsset";

    auto* obj = CreateDObject<DTestObjectA>();
    const ObjectId otherObjectId = ObjectId::Generate();
    obj->SetObjectId(otherObjectId);
    obj->m_name = "BeforeRename";
    asset->AddObject(obj);

    const auto path = m_tempDir / "OtherAsset.dasset.json";
    m_core->GetAssetDatabase()->CreateAsset(path, asset);
    ASSERT_NE(otherAssetId, GetActiveSceneAssetId());

    auto dispatchRes = Dispatch("common", "RenameObject",
                                {{"assetId",  otherAssetId.ToString()},
                                 {"objectId", otherObjectId.ToString()},
                                 {"newName",  "AfterRename"}});
    EXPECT_TRUE(dispatchRes["ok"].get<bool>());
    EXPECT_TRUE(dispatchRes.value("queued", false));

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_TRUE(json::parse(responses[0])["ok"].get<bool>());

    auto* resolved = dynamic_cast<DTestObjectA*>(
        m_core->ResolveObject(otherAssetId, otherObjectId));
    ASSERT_NE(resolved, nullptr);
    EXPECT_EQ(resolved->m_name, "AfterRename");
}

TEST_F(McpCommonSystemTests, CommandSetProperty_WithExplicitAssetId_MutatesObjectInOtherAsset)
{
    auto* asset = CreateDObject<PA_TestAsset>();
    const AssetId otherAssetId = AssetId::Generate();
    asset->GetHeader().m_persistentId = otherAssetId;
    asset->GetHeader().m_className = "PA_TestAsset";

    auto* obj = CreateDObject<DTestObjectA>();
    const ObjectId otherObjectId = ObjectId::Generate();
    obj->SetObjectId(otherObjectId);
    obj->m_health = 100.0f;
    asset->AddObject(obj);

    const auto path = m_tempDir / "OtherAssetForProperty.dasset.json";
    m_core->GetAssetDatabase()->CreateAsset(path, asset);
    ASSERT_NE(otherAssetId, GetActiveSceneAssetId());

    auto dispatchRes = Dispatch("common", "SetProperty",
                                {{"assetId",     otherAssetId.ToString()},
                                 {"objectId",    otherObjectId.ToString()},
                                 {"propertyName", "m_health"},
                                 {"valueAfter",   42.5f}});
    EXPECT_TRUE(dispatchRes["ok"].get<bool>());
    EXPECT_TRUE(dispatchRes.value("queued", false));

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_TRUE(json::parse(responses[0])["ok"].get<bool>());

    auto* resolved = dynamic_cast<DTestObjectA*>(
        m_core->ResolveObject(otherAssetId, otherObjectId));
    ASSERT_NE(resolved, nullptr);
    EXPECT_NEAR(resolved->m_health, 42.5f, 0.001f);
}

TEST_F(McpCommonSystemTests, CommandSaveProject_QueuesAndDrains)
{
    auto dispatchRes = Dispatch("common", "SaveProject");
    EXPECT_TRUE(dispatchRes["ok"].get<bool>());
    EXPECT_TRUE(dispatchRes.value("queued", false));

    std::vector<std::string> responses;
    m_core->DrainCommandQueue(responses);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_TRUE(json::parse(responses[0])["ok"].get<bool>());
}
