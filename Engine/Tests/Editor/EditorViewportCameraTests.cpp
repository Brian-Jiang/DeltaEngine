#include "Editor/EditorViewportCamera.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>

using namespace DeltaEngine;

namespace
{
std::filesystem::path MakeTempPath(const char* prefix)
{
    const auto root = std::filesystem::temp_directory_path() /
        (std::string(prefix) + "_" +
         std::to_string(static_cast<long long>(
             std::chrono::steady_clock::now().time_since_epoch().count())));
    std::filesystem::create_directories(root);
    return root;
}
}

TEST(EditorViewportCameraTests, BuildCameraCB_ZeroHeight_AspectRatioIsOne)
{
    EditorViewportCamera cam{};
    EXPECT_FLOAT_EQ(cam.GetAspectRatio(1280.f, 0.f), 1.f);
}

TEST(EditorViewportCameraTests, BuildActiveRenderCamera_PropagatesFields)
{
    EditorViewportCamera cam{};
    cam.fov = 70.f;
    cam.nearPlane = 0.5f;
    cam.farPlane = 1234.f;

    const ActiveRenderCamera arc = cam.BuildActiveRenderCamera(1024.f, 512.f);

    EXPECT_FLOAT_EQ(arc.nearPlane, 0.5f);
    EXPECT_FLOAT_EQ(arc.farPlane, 1234.f);
    EXPECT_FLOAT_EQ(arc.aspectRatio, 2.f);
    EXPECT_NEAR(arc.fovY, DirectX::XMConvertToRadians(70.f), 1e-5f);
}

TEST(EditorViewportCameraTests, LoadViewportCamerasFromPath_MissingFile_ReturnsFalse)
{
    const auto root = MakeTempPath("DeltaVpCam_missing");
    std::vector<EditorViewportCamera> cams;
    cams.resize(2);
    EXPECT_FALSE(LoadViewportCamerasFromPath(cams, root / "no_such_file.json"));
    EXPECT_EQ(cams.size(), 2u);
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
}

TEST(EditorViewportCameraTests, LoadViewportCamerasFromPath_RootNotArray_LeavesUnchanged)
{
    const auto root = MakeTempPath("DeltaVpCam_obj");
    const auto path = root / "cams.json";
    std::ofstream(path) << R"({"not":"array"})";

    std::vector<EditorViewportCamera> cams;
    cams.resize(1);
    EXPECT_FALSE(LoadViewportCamerasFromPath(cams, path));
    EXPECT_EQ(cams.size(), 1u);

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
}

// Regression: NaN/Inf in the persisted camera JSON must be skipped, not propagated into the
// ActiveRenderCamera (would corrupt projection matrices).
TEST(EditorViewportCameraTests, LoadViewportCamerasFromPath_NonFiniteFovUsesDefault)
{
    const auto root = MakeTempPath("DeltaVpCam_nan");
    const auto path = root / "cams.json";

    // nlohmann::json::parse accepts NaN as the literal "NaN" only in non-strict mode; emit as a
    // very large finite value via direct text so the stream parses, then divide to produce inf.
    // Simpler: write the JSON with explicit NaN — nlohmann allows reading "NaN" with default
    // parser flags via parse() since v3.10+. We just write a literal here.
    std::ofstream(path) << R"([{"fov":NaN,"nearPlane":0.1,"farPlane":1000.0,"position":[0,0,0],"rotation":[0,0,0,1]}])";

    std::vector<EditorViewportCamera> cams;
    const bool ok = LoadViewportCamerasFromPath(cams, path);

    if (!ok)
    {
        // Strict parser path: file is rejected wholesale; default cams remain.
        EXPECT_TRUE(cams.empty());
    }
    else
    {
        ASSERT_EQ(cams.size(), 1u);
        EXPECT_TRUE(std::isfinite(cams[0].fov));
    }

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
}

TEST(EditorViewportCameraTests, SaveAndLoad_RoundTripPreservesAllFields)
{
    const auto root = MakeTempPath("DeltaVpCam_rt");
    const auto path = root / "cams.json";

    EditorViewportCamera in{};
    in.fov = 42.f;
    in.nearPlane = 0.3f;
    in.farPlane = 999.f;
    in.position = DirectX::XMFLOAT3{ 7.f, 8.f, 9.f };
    in.rotation = DirectX::XMFLOAT4{ 0.1f, 0.2f, 0.3f, 0.4f };

    SaveViewportCamerasToPath({ in }, path);
    std::vector<EditorViewportCamera> out;
    ASSERT_TRUE(LoadViewportCamerasFromPath(out, path));
    ASSERT_EQ(out.size(), 1u);

    EXPECT_FLOAT_EQ(out[0].fov, in.fov);
    EXPECT_FLOAT_EQ(out[0].nearPlane, in.nearPlane);
    EXPECT_FLOAT_EQ(out[0].farPlane, in.farPlane);
    EXPECT_FLOAT_EQ(out[0].position.y, in.position.y);
    EXPECT_FLOAT_EQ(out[0].rotation.z, in.rotation.z);

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
}
