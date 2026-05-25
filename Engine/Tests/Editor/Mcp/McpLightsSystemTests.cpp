// Engine/Tests/Editor/Mcp/McpLightsSystemTests.cpp
#include "Editor/Mcp/McpCoreFixture.h"

#include "Editor/Commands/EditorCommandManager.h"

#include "Runtime/Core/UUID.h"
#include "Runtime/Graphics/Light/LightComponent.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class McpLightsSystemTests : public McpCoreFixture
{
protected:
    // Creates a GameObject + PointLight component; returns the PointLight objectId.
    std::string CreatePointLight()
    {
        const std::string goId = CreateLegacyGameObject();
        if (goId.empty())
            return {};

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
        if (r.empty())
            return {};
        return json::parse(r[0]).value("objectId", std::string{});
    }
};

// SetIntensity: immediately applies the value in headless mode (no animation manager)
TEST_F(McpLightsSystemTests, SetIntensity_Headless_ImmediatelyAppliesValue)
{
    const std::string plId = CreatePointLight();
    if (plId.empty())
        GTEST_SKIP() << "Failed to create PointLight";

    const AssetId assetId   = GetActiveSceneAssetId();
    const ObjectId objectId = DeltaEngine::UUID::FromString(plId);

    auto res = Dispatch("lights", "SetIntensity",
                        {{"assetId",          assetId.ToString()},
                         {"objectId",         plId},
                         {"value",            5.0f},
                         {"duration_seconds", 2.0f}});  // duration ignored in headless
    EXPECT_TRUE(res["ok"].get<bool>());

    LightComponent* light = m_core->ResolveObject<LightComponent>(assetId, objectId);
    ASSERT_NE(light, nullptr);
    EXPECT_NEAR(light->GetIntensity(), 5.0f, 0.001f);
}

// SetIntensity: pushes a command to the undo stack
TEST_F(McpLightsSystemTests, SetIntensity_PushesCommandToUndoStack)
{
    const std::string plId = CreatePointLight();
    if (plId.empty())
        GTEST_SKIP() << "Failed to create PointLight";

    const AssetId assetId = GetActiveSceneAssetId();

    // Record undo stack depth before
    const size_t undoDepthBefore = m_core->GetCommandManager().GetUndoStackDepth();

    auto res = Dispatch("lights", "SetIntensity",
                        {{"assetId",  assetId.ToString()},
                         {"objectId", plId},
                         {"value",    3.0f}});
    EXPECT_TRUE(res["ok"].get<bool>());

    EXPECT_GT(m_core->GetCommandManager().GetUndoStackDepth(), undoDepthBefore);
}

// SetIntensity + Undo: reverts the intensity
TEST_F(McpLightsSystemTests, SetIntensity_Undo_RevertsIntensity)
{
    const std::string plId = CreatePointLight();
    if (plId.empty())
        GTEST_SKIP() << "Failed to create PointLight";

    const AssetId  assetId  = GetActiveSceneAssetId();
    const ObjectId objectId = DeltaEngine::UUID::FromString(plId);

    LightComponent* light = m_core->ResolveObject<LightComponent>(assetId, objectId);
    ASSERT_NE(light, nullptr);
    const float originalIntensity = light->GetIntensity();

    auto res = Dispatch("lights", "SetIntensity",
                        {{"assetId",  assetId.ToString()},
                         {"objectId", plId},
                         {"value",    99.0f}});
    EXPECT_TRUE(res["ok"].get<bool>());
    EXPECT_NEAR(light->GetIntensity(), 99.0f, 0.001f);

    auto undoRes = Dispatch("undo_history", "Undo");
    EXPECT_TRUE(undoRes["ok"].get<bool>());

    EXPECT_NEAR(light->GetIntensity(), originalIntensity, 0.001f);
}

// SetIntensity: invalid objectId returns error
TEST_F(McpLightsSystemTests, SetIntensity_InvalidObjectId_ReturnsError)
{
    const AssetId assetId = GetActiveSceneAssetId();
    auto res = Dispatch("lights", "SetIntensity",
                        {{"assetId",  assetId.ToString()},
                         {"objectId", "00000000-0000-0000-0000-000000000000"},
                         {"value",    1.0f}});
    EXPECT_FALSE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("error"));
}

// SetIntensity: missing params returns error
TEST_F(McpLightsSystemTests, SetIntensity_MissingParams_ReturnsError)
{
    auto res = Dispatch("lights", "SetIntensity");
    EXPECT_FALSE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("error"));
}
