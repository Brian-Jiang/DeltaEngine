#include "Editor/EditorSelectionState.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(EditorSelectionStateTests, SetSelectionClearsCacheAndStoresIds)
{
    EditorSelectionState state;
    const AssetId a  = AssetId::Generate();
    const ObjectId o = ObjectId::Generate();

    state.SetSelection(a, o);
    EXPECT_EQ(state.GetSelectedAssetId(), a);
    EXPECT_EQ(state.GetSelectedObjectId(), o);
    EXPECT_TRUE(state.HasSelection());
    EXPECT_FALSE(state.HasAssetSelection());
}

TEST(EditorSelectionStateTests, SelectAssetUsesObjectIdNullSentinel)
{
    EditorSelectionState state;
    const AssetId a = AssetId::Generate();

    state.SelectAsset(a);
    EXPECT_EQ(state.GetSelectedAssetId(), a);
    EXPECT_TRUE(state.GetSelectedObjectId().IsNull());
    EXPECT_TRUE(state.HasSelection());
    EXPECT_TRUE(state.HasAssetSelection());
}

TEST(EditorSelectionStateTests, ClearSelectionZerosIds)
{
    EditorSelectionState state;
    state.SetSelection(AssetId::Generate(), ObjectId::Generate());
    state.ClearSelection();

    EXPECT_FALSE(state.HasSelection());
    EXPECT_TRUE(state.GetSelectedAssetId().IsNull());
    EXPECT_TRUE(state.GetSelectedObjectId().IsNull());
}

TEST(EditorSelectionStateTests, NotifyObjectDestroyedClearsWhenObjectIdMatches)
{
    EditorSelectionState state;
    const ObjectId o = ObjectId::Generate();
    state.SetSelection(AssetId::Generate(), o);

    state.NotifyObjectDestroyed(o);
    EXPECT_FALSE(state.HasSelection());
}

TEST(EditorSelectionStateTests, NotifyObjectDestroyedIgnoresOtherIds)
{
    EditorSelectionState state;
    const ObjectId o = ObjectId::Generate();
    state.SetSelection(AssetId::Generate(), o);

    state.NotifyObjectDestroyed(ObjectId::Generate());
    EXPECT_TRUE(state.HasSelection());
}

TEST(EditorSelectionStateTests, HasSelectionTracksEmptyAndAssetAndObjectStates)
{
    EditorSelectionState state;

    EXPECT_FALSE(state.HasSelection());

    state.SelectAsset(AssetId::Generate());
    EXPECT_TRUE(state.HasSelection());

    state.ClearSelection();
    EXPECT_FALSE(state.HasSelection());

    state.SetSelection(AssetId::Generate(), ObjectId::Generate());
    EXPECT_TRUE(state.HasSelection());
}
