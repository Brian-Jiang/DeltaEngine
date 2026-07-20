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

        return ExecCreateComponent(goId, "PointLight");
    }

    // Dispatch a lights command synchronously; returns its real result.
    json DispatchAndDrain(const std::string& op, const json& params = json::object())
    {
        return Dispatch("lights", op, params);
    }
};

// SetIntensity: immediately applies the value in headless mode (no animation manager).
// With duration > 0 but no animation manager, it falls back to SetProperty.
TEST_F(McpLightsSystemTests, SetIntensity_Headless_ImmediatelyAppliesValue)
{
    const std::string plId = CreatePointLight();
    if (plId.empty())
        GTEST_SKIP() << "Failed to create PointLight";

    const AssetId  assetId  = GetActiveSceneAssetId();
    const ObjectId objectId = DeltaEngine::UUID::FromString(plId);

    // Dispatch enqueues; drain executes on "main thread".
    const auto res = DispatchAndDrain("SetIntensity",
                        {{"assetId",          assetId.ToString()},
                         {"objectId",         plId},
                         {"value",            5.0f},
                         {"duration_seconds", 2.0f}});  // duration > 0, but headless -> fallback
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
    const size_t undoDepthBefore = m_core->GetCommandManager().GetUndoStackDepth();

    DispatchAndDrain("SetIntensity",
                     {{"assetId",  assetId.ToString()},
                      {"objectId", plId},
                      {"value",    3.0f}});

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

    DispatchAndDrain("SetIntensity",
                     {{"assetId",  assetId.ToString()},
                      {"objectId", plId},
                      {"value",    99.0f}});
    EXPECT_NEAR(light->GetIntensity(), 99.0f, 0.001f);

    auto undoRes = Dispatch("undo_history", "Undo");
    EXPECT_TRUE(undoRes["ok"].get<bool>());

    EXPECT_NEAR(light->GetIntensity(), originalIntensity, 0.001f);
}

// SetIntensity: null UUID for objectId is caught before enqueue and returns an immediate error
TEST_F(McpLightsSystemTests, SetIntensity_InvalidObjectId_ReturnsError)
{
    const AssetId assetId = GetActiveSceneAssetId();
    // "00000000-..." parses to the null UUID — caught immediately by IsNull() check.
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
