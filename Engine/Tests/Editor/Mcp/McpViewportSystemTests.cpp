#include "Editor/Mcp/McpCoreFixture.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class McpViewportSystemTests : public McpCoreFixture {};

TEST_F(McpViewportSystemTests, QueryCamera_ReturnsPositionAndFov)
{
    auto res = Dispatch("viewport", "camera");
    EXPECT_TRUE(res["ok"].get<bool>());
    EXPECT_TRUE(res["position"].is_array());
    EXPECT_EQ(res["position"].size(), 3u);
    EXPECT_TRUE(res.contains("fov"));
}

TEST_F(McpViewportSystemTests, QueryRenderSettings_ReturnsOk)
{
    auto res = Dispatch("viewport", "render_settings");
    EXPECT_TRUE(res["ok"].get<bool>());
    // In headless mode a note field is added instead of render dimensions
    EXPECT_TRUE(res.contains("note") || res.contains("scene_render_width"));
}

TEST_F(McpViewportSystemTests, QueryVisibleObjects_ReturnsObjectIdsArray)
{
    auto res = Dispatch("viewport", "visible_objects");
    EXPECT_TRUE(res["ok"].get<bool>());
    EXPECT_TRUE(res["object_ids"].is_array());
}

TEST_F(McpViewportSystemTests, CommandSetViewportCamera_ReturnsOk)
{
    auto res = Dispatch("viewport", "SetViewportCamera",
                        {{"position", json::array({10.f, 5.f, -3.f})},
                         {"fov", 75.0f}});
    EXPECT_TRUE(res["ok"].get<bool>());

    // Verify the new position was applied
    auto cam = Dispatch("viewport", "camera");
    EXPECT_TRUE(cam["ok"].get<bool>());
    EXPECT_NEAR(cam["position"][0].get<float>(), 10.f, 0.001f);
    EXPECT_NEAR(cam["position"][1].get<float>(),  5.f, 0.001f);
    EXPECT_NEAR(cam["fov"].get<float>(), 75.f, 0.001f);
}

TEST_F(McpViewportSystemTests, CommandSetViewportCamera_ZeroDurationAppliesImmediately)
{
    auto res = Dispatch("viewport", "SetViewportCamera",
                        {{"position", json::array({-4.f, 8.f, 12.f})},
                         {"fov", 33.0f},
                         {"duration_seconds", 0.0f}});
    EXPECT_TRUE(res["ok"].get<bool>());

    auto cam = Dispatch("viewport", "camera");
    EXPECT_NEAR(cam["position"][0].get<float>(), -4.f, 0.001f);
    EXPECT_NEAR(cam["position"][2].get<float>(), 12.f, 0.001f);
    EXPECT_NEAR(cam["fov"].get<float>(), 33.f, 0.001f);
}

// A 3-element rotation is euler DEGREES, matching scene/SetRotation.
TEST_F(McpViewportSystemTests, CommandSetViewportCamera_EulerRotationIsDegrees)
{
    auto res = Dispatch("viewport", "SetViewportCamera",
                        {{"rotation", json::array({30.f, 0.f, 0.f})},
                         {"duration_seconds", 0.0f}});
    EXPECT_TRUE(res["ok"].get<bool>());

    auto cam = Dispatch("viewport", "camera");
    // 30 degrees of pitch: q = (sin(15deg), 0, 0, cos(15deg))
    EXPECT_NEAR(cam["rotation"][0].get<float>(), 0.258819f, 0.001f);
    EXPECT_NEAR(cam["rotation"][1].get<float>(), 0.f,       0.001f);
    EXPECT_NEAR(cam["rotation"][2].get<float>(), 0.f,       0.001f);
    EXPECT_NEAR(cam["rotation"][3].get<float>(), 0.965926f, 0.001f);

    EXPECT_NEAR(cam["euler"][0].get<float>(), 30.f, 0.01f);
    EXPECT_NEAR(cam["euler"][1].get<float>(),  0.f, 0.01f);
    EXPECT_NEAR(cam["euler"][2].get<float>(),  0.f, 0.01f);
}

TEST_F(McpViewportSystemTests, CommandSetViewportCamera_QuaternionRotationRoundTrips)
{
    auto res = Dispatch("viewport", "SetViewportCamera",
                        {{"rotation", json::array({0.f, 0.382683f, 0.f, 0.923880f})},
                         {"duration_seconds", 0.0f}});
    EXPECT_TRUE(res["ok"].get<bool>());

    auto cam = Dispatch("viewport", "camera");
    EXPECT_NEAR(cam["rotation"][1].get<float>(), 0.382683f, 0.001f);
    EXPECT_NEAR(cam["rotation"][3].get<float>(), 0.923880f, 0.001f);
    EXPECT_NEAR(cam["euler"][1].get<float>(), 45.f, 0.01f);
}

// The fly camera clamps pitch to +/-89; an MCP-set target must not exceed it either.
TEST_F(McpViewportSystemTests, CommandSetViewportCamera_ClampsPitchToFlyCameraLimit)
{
    auto res = Dispatch("viewport", "SetViewportCamera",
                        {{"rotation", json::array({89.9f, 0.f, 0.f})},
                         {"duration_seconds", 0.0f}});
    EXPECT_TRUE(res["ok"].get<bool>());

    auto cam = Dispatch("viewport", "camera");
    EXPECT_NEAR(cam["euler"][0].get<float>(), 89.f, 0.01f);
}

// Headless has no EditorAnimationManager, so even the animated default must apply synchronously.
TEST_F(McpViewportSystemTests, CommandSetViewportCamera_DefaultDurationAppliesInHeadless)
{
    ASSERT_EQ(m_core->GetAnimationManager(), nullptr);

    auto res = Dispatch("viewport", "SetViewportCamera",
                        {{"position", json::array({1.5f, 2.5f, 3.5f})}});
    EXPECT_TRUE(res["ok"].get<bool>());

    auto cam = Dispatch("viewport", "camera");
    EXPECT_NEAR(cam["position"][0].get<float>(), 1.5f, 0.001f);
    EXPECT_NEAR(cam["position"][1].get<float>(), 2.5f, 0.001f);
    EXPECT_NEAR(cam["position"][2].get<float>(), 3.5f, 0.001f);
}
