#include "Editor/EditorViewportCamera.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>

using namespace DeltaEngine;

TEST(EditorViewportCameraPersistenceTests, SaveViewportCamerasToPath_RoundTrip_PreservesFovAndPosition)
{
    const auto root =
        std::filesystem::temp_directory_path() /
        ("DeltaVpCam_" +
            std::to_string(static_cast<long long>(
                std::chrono::steady_clock::now().time_since_epoch().count())));
    std::filesystem::create_directories(root);
    const auto path = root / "viewport_cameras.json";

    EditorViewportCamera in{};
    in.fov               = 55.f;
    in.nearPlane         = 0.25f;
    in.farPlane          = 5000.f;
    in.position          = DirectX::XMFLOAT3{ 1.f, 2.f, 3.f };
    in.rotation          = DirectX::XMFLOAT4{ 0.f, 0.f, 0.f, 1.f };

    SaveViewportCamerasToPath(std::vector<EditorViewportCamera>{ in }, path);
    ASSERT_TRUE(std::filesystem::exists(path));

    std::vector<EditorViewportCamera> out;
    ASSERT_TRUE(LoadViewportCamerasFromPath(out, path));
    ASSERT_EQ(out.size(), 1u);

    EXPECT_FLOAT_EQ(out[0].fov, in.fov);
    EXPECT_FLOAT_EQ(out[0].nearPlane, in.nearPlane);
    EXPECT_FLOAT_EQ(out[0].farPlane, in.farPlane);
    EXPECT_FLOAT_EQ(out[0].position.x, in.position.x);
    EXPECT_FLOAT_EQ(out[0].position.y, in.position.y);
    EXPECT_FLOAT_EQ(out[0].position.z, in.position.z);
    EXPECT_FLOAT_EQ(out[0].rotation.x, in.rotation.x);
    EXPECT_FLOAT_EQ(out[0].rotation.w, in.rotation.w);

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
}

// Regression: previously LoadViewportCameras swallowed parse errors — invalid JSON must leave output unchanged on failure paths.
TEST(EditorViewportCameraPersistenceTests, LoadViewportCamerasFromPath_NotJson_NoOverwrite)
{
    const auto root =
        std::filesystem::temp_directory_path() /
        ("DeltaVpCam_bad_" +
            std::to_string(static_cast<long long>(
                std::chrono::steady_clock::now().time_since_epoch().count())));
    std::filesystem::create_directories(root);
    const auto path = root / "bad.json";

    std::fstream f(path, std::ios::out | std::ios::trunc);
    f << "not-json";
    f.close();

    std::vector<EditorViewportCamera> out;
    out.resize(3);
    EXPECT_FALSE(LoadViewportCamerasFromPath(out, path));
    EXPECT_EQ(out.size(), 3u);

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
}
