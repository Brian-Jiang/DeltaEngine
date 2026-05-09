#include "Editor/EditorWindows/EditorAssetBrowserPaths.h"

#include <fstream>

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>

using namespace DeltaEngine;

namespace
{
std::filesystem::path MakeAsciiTempUnderRoot(const std::filesystem::path& root, const std::filesystem::path& rel)
{
    const auto absolute = root / rel / "AsciiSub";
    std::filesystem::create_directories(absolute);
    std::filesystem::create_directories(absolute / "nested");
    return absolute.lexically_normal();
}

std::filesystem::path ScopedTempAssetsRoot()
{
    static int s_suffix = 0;
    const auto base = std::filesystem::temp_directory_path() /
        ("DeltaAssetBrowserPath_" + std::to_string(s_suffix++));
    std::filesystem::create_directories(base);
    return base;
}
}

TEST(EditorAssetBrowserPathTests, NormalizeEditorPath_Exists_IsAbsolute)
{
    const std::filesystem::path root = ScopedTempAssetsRoot();
    const auto leaf = root / "leaf";
    std::filesystem::create_directories(leaf);

    const auto n = NormalizeEditorPath(leaf);
    EXPECT_TRUE(std::filesystem::exists(n));
}

TEST(EditorAssetBrowserPathTests, IsSameOrChildPathNormalized_Child_ReturnsTrue)
{
    const std::filesystem::path root = ScopedTempAssetsRoot();
    const auto child = MakeAsciiTempUnderRoot(root, "p");

    EXPECT_TRUE(IsSameOrChildPathNormalized(child, root));
    EXPECT_TRUE(IsSameOrChildPathNormalized(child / "nested", root));
}

TEST(EditorAssetBrowserPathTests, IsSameOrChildPathNormalized_Parent_NotChildOfDeepPath)
{
    const std::filesystem::path root = ScopedTempAssetsRoot();
    const auto other = root / "other";
    const auto deep  = other / "x";
    std::filesystem::create_directories(deep);

    EXPECT_FALSE(IsSameOrChildPathNormalized(root, deep));
}

TEST(EditorAssetBrowserPathTests, ToAssetRootRelativeString_SpacesInNames_RoundTrips)
{
    const std::filesystem::path assetRoot = ScopedTempAssetsRoot();
    const auto sub = assetRoot / "folder with spaces" / "child";
    std::filesystem::create_directories(sub);
    const std::string rel = ToAssetRootRelativeString(sub, assetRoot);
    ASSERT_FALSE(rel.empty());
    const auto rebuilt = (assetRoot / std::filesystem::path(rel)).lexically_normal();
    EXPECT_EQ(rebuilt, sub.lexically_normal());
}

TEST(EditorAssetBrowserPathTests, UniqueDirUnderParent_WhenExists_AppendsSuffix)
{
    const std::filesystem::path root = ScopedTempAssetsRoot();
    std::filesystem::create_directories(root / "Base");
    const auto u = UniqueDirUnderParent(root, "Base");
    EXPECT_NE(u, root / "Base");
    EXPECT_EQ(u.filename().string().rfind("Base", 0), 0u);
}

TEST(EditorAssetBrowserPathTests, UniqueAssetPathInFolder_WhenTaken_AppendsSuffix)
{
    const std::filesystem::path root = ScopedTempAssetsRoot();
    std::filesystem::create_directories(root);
    {
        std::ofstream f(root / "foo.dasset.json");
        f << "{}";
    }
    const auto p = UniqueAssetPathInFolder(root, "foo");
    EXPECT_NE(p, root / "foo.dasset.json");
    EXPECT_TRUE(p.filename().string().starts_with("foo_"));
}

TEST(EditorAssetBrowserPathTests, GetAssetDisplayNameForBrowser_StripsDassetJsonStem)
{
    EXPECT_EQ(GetAssetDisplayNameForBrowser(std::filesystem::path("Mesh") / "foo.dasset.json"), "foo");
}
