#include "Editor/EditorSessionState.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

using namespace DeltaEngine;

namespace
{
std::filesystem::path MakeTempRoot()
{
    const auto root =
        std::filesystem::temp_directory_path() /
        ("DeltaSessionState_" +
            std::to_string(static_cast<long long>(
                std::chrono::steady_clock::now().time_since_epoch().count())));
    std::filesystem::create_directories(root);
    return root;
}
}

TEST(EditorSessionStateTests, SaveAndLoad_RoundTrip_PreservesLastScenePath)
{
    const auto root = MakeTempRoot();
    const auto path = root / "editor_session.json";

    EditorSessionState in;
    in.lastScenePath = "D:/Projects/DeltaEngine/ImportedAssets/MyScene.dasset.json";

    SaveEditorSessionStateToPath(in, path);
    ASSERT_TRUE(std::filesystem::exists(path));

    EditorSessionState out;
    ASSERT_TRUE(LoadEditorSessionStateFromPath(out, path));
    EXPECT_EQ(out.lastScenePath, in.lastScenePath);

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
}

TEST(EditorSessionStateTests, Load_MissingFile_ReturnsFalseAndLeavesStateUnchanged)
{
    const auto root = MakeTempRoot();
    const auto path = root / "does_not_exist.json";

    EditorSessionState state;
    state.lastScenePath = "untouched";
    EXPECT_FALSE(LoadEditorSessionStateFromPath(state, path));
    EXPECT_EQ(state.lastScenePath, "untouched");

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
}

TEST(EditorSessionStateTests, Load_MalformedJson_ReturnsFalse)
{
    const auto root = MakeTempRoot();
    const auto path = root / "editor_session.json";
    {
        std::ofstream out(path);
        out << "not valid json {{{";
    }

    EditorSessionState state;
    state.lastScenePath = "untouched";
    EXPECT_FALSE(LoadEditorSessionStateFromPath(state, path));
    EXPECT_EQ(state.lastScenePath, "untouched");

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
}
