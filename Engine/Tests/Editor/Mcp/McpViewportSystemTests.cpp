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
