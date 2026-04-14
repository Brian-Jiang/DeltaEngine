#include "Editor/Mcp/McpCoreFixture.h"

#include "Editor/Commands/PropertyValueIO.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"

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
