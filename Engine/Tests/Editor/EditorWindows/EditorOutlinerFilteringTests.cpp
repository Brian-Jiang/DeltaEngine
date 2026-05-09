#include "Editor/EditorWindows/EditorOutlinerFiltering.h"
#include "Editor/EditorWindows/EditorWindow_WorldOutliner.h"

#include <gtest/gtest.h>

#include <vector>

using namespace DeltaEngine;

TEST(EditorOutlinerFilteringTests, BuildOutlinerFilterMatches_EmptyFilter_ReturnsAll)
{
    std::vector<OutlinerEntry> entries;
    OutlinerEntry a{};
    a.index = 0;
    a.name  = "Alpha";
    a.go    = nullptr;
    entries.push_back(a);

    OutlinerEntry b{};
    b.index = 1;
    b.name  = "BetaGamma";
    b.go    = nullptr;
    entries.push_back(b);

    std::vector<const OutlinerEntry*> filtered;
    BuildOutlinerFilterMatches(entries, "", filtered);
    ASSERT_EQ(filtered.size(), 2u);
}

TEST(EditorOutlinerFilteringTests, BuildOutlinerFilterMatches_SubstringCaseInsensitive_SelectsSubset)
{
    std::vector<OutlinerEntry> entries;
    OutlinerEntry a{};
    a.name = "MainCamera";
    entries.push_back(a);
    OutlinerEntry b{};
    b.name = "floor_mesh";
    entries.push_back(b);

    char buf[]{"cam"};
    std::vector<const OutlinerEntry*> filtered;
    BuildOutlinerFilterMatches(entries, buf, filtered);
    ASSERT_EQ(filtered.size(), 1u);
    EXPECT_EQ(filtered.front()->name, "MainCamera");
}

TEST(EditorOutlinerFilteringTests, BuildOutlinerFilterMatches_NoMatch_ReturnsEmpty)
{
    std::vector<OutlinerEntry> entries;
    OutlinerEntry a{};
    a.name = "Sole";
    entries.push_back(a);

    char buf[]{"zzz"};
    std::vector<const OutlinerEntry*> filtered;
    BuildOutlinerFilterMatches(entries, buf, filtered);
    EXPECT_TRUE(filtered.empty());
}
