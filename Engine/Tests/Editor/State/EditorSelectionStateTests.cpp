#include "Editor/EditorSelectionState.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

// --- GameObject selection ---

TEST(EditorSelectionStateTests, SetSelectedGameObjectReplacesPreviousAndClearsAssets)
{
    EditorSelectionState state;
    const ObjectId go1 = ObjectId::Generate();
    const ObjectId go2 = ObjectId::Generate();
    const AssetId  a1  = AssetId::Generate();

    state.SetSelectedAsset(a1);
    EXPECT_TRUE(state.HasAssetSelection());

    state.SetSelectedGameObject(go1);
    EXPECT_TRUE(state.HasGameObjectSelection());
    EXPECT_FALSE(state.HasAssetSelection());
    EXPECT_TRUE(state.IsGameObjectSelected(go1));
    EXPECT_EQ(state.GetSelectedGameObjects().size(), 1u);

    state.SetSelectedGameObject(go2);
    EXPECT_FALSE(state.IsGameObjectSelected(go1));
    EXPECT_TRUE(state.IsGameObjectSelected(go2));
    EXPECT_EQ(state.GetSelectedGameObjects().size(), 1u);
}

TEST(EditorSelectionStateTests, AddSelectedGameObjectAppends)
{
    EditorSelectionState state;
    const ObjectId go1 = ObjectId::Generate();
    const ObjectId go2 = ObjectId::Generate();

    state.SetSelectedGameObject(go1);
    state.AddSelectedGameObject(go2);

    EXPECT_EQ(state.GetSelectedGameObjects().size(), 2u);
    EXPECT_TRUE(state.IsGameObjectSelected(go1));
    EXPECT_TRUE(state.IsGameObjectSelected(go2));
}

TEST(EditorSelectionStateTests, AddSelectedGameObjectSkipsDuplicate)
{
    EditorSelectionState state;
    const ObjectId go1 = ObjectId::Generate();

    state.SetSelectedGameObject(go1);
    state.AddSelectedGameObject(go1);

    EXPECT_EQ(state.GetSelectedGameObjects().size(), 1u);
}

TEST(EditorSelectionStateTests, AddSelectedGameObjectClearsAssets)
{
    EditorSelectionState state;
    state.SetSelectedAsset(AssetId::Generate());
    EXPECT_TRUE(state.HasAssetSelection());

    state.AddSelectedGameObject(ObjectId::Generate());
    EXPECT_FALSE(state.HasAssetSelection());
}

TEST(EditorSelectionStateTests, RemoveSelectedGameObject)
{
    EditorSelectionState state;
    const ObjectId go1 = ObjectId::Generate();
    const ObjectId go2 = ObjectId::Generate();

    state.SetSelectedGameObject(go1);
    state.AddSelectedGameObject(go2);
    state.RemoveSelectedGameObject(go1);

    EXPECT_FALSE(state.IsGameObjectSelected(go1));
    EXPECT_TRUE(state.IsGameObjectSelected(go2));
    EXPECT_EQ(state.GetSelectedGameObjects().size(), 1u);
}

TEST(EditorSelectionStateTests, ClearGameObjectSelection)
{
    EditorSelectionState state;
    state.SetSelectedGameObject(ObjectId::Generate());
    state.ClearGameObjectSelection();
    EXPECT_FALSE(state.HasGameObjectSelection());
}

// --- Asset selection ---

TEST(EditorSelectionStateTests, SetSelectedAssetClearsGameObjects)
{
    EditorSelectionState state;
    state.SetSelectedGameObject(ObjectId::Generate());
    EXPECT_TRUE(state.HasGameObjectSelection());

    state.SetSelectedAsset(AssetId::Generate());
    EXPECT_FALSE(state.HasGameObjectSelection());
    EXPECT_TRUE(state.HasAssetSelection());
}

TEST(EditorSelectionStateTests, AddSelectedAssetAppends)
{
    EditorSelectionState state;
    const AssetId a1 = AssetId::Generate();
    const AssetId a2 = AssetId::Generate();

    state.SetSelectedAsset(a1);
    state.AddSelectedAsset(a2);

    EXPECT_EQ(state.GetSelectedAssets().size(), 2u);
    EXPECT_TRUE(state.IsAssetSelected(a1));
    EXPECT_TRUE(state.IsAssetSelected(a2));
}

TEST(EditorSelectionStateTests, RemoveSelectedAsset)
{
    EditorSelectionState state;
    const AssetId a1 = AssetId::Generate();
    const AssetId a2 = AssetId::Generate();

    state.SetSelectedAsset(a1);
    state.AddSelectedAsset(a2);
    state.RemoveSelectedAsset(a1);

    EXPECT_FALSE(state.IsAssetSelected(a1));
    EXPECT_TRUE(state.IsAssetSelected(a2));
}

TEST(EditorSelectionStateTests, ClearAssetSelection)
{
    EditorSelectionState state;
    state.SetSelectedAsset(AssetId::Generate());
    state.ClearAssetSelection();
    EXPECT_FALSE(state.HasAssetSelection());
}

// --- Component selection (independent) ---

TEST(EditorSelectionStateTests, ComponentSelectionDoesNotClearGameObjects)
{
    EditorSelectionState state;
    const ObjectId go = ObjectId::Generate();
    const ObjectId co = ObjectId::Generate();

    state.SetSelectedGameObject(go);
    state.SetSelectedComponent(co);

    EXPECT_TRUE(state.HasGameObjectSelection());
    EXPECT_TRUE(state.HasComponentSelection());
    EXPECT_TRUE(state.IsGameObjectSelected(go));
    EXPECT_TRUE(state.IsComponentSelected(co));
}

TEST(EditorSelectionStateTests, GameObjectSelectionDoesNotClearComponents)
{
    EditorSelectionState state;
    const ObjectId co = ObjectId::Generate();

    state.SetSelectedComponent(co);
    state.SetSelectedGameObject(ObjectId::Generate());

    EXPECT_TRUE(state.HasComponentSelection());
    EXPECT_TRUE(state.IsComponentSelected(co));
}

TEST(EditorSelectionStateTests, AddSelectedComponentAppends)
{
    EditorSelectionState state;
    const ObjectId c1 = ObjectId::Generate();
    const ObjectId c2 = ObjectId::Generate();

    state.SetSelectedComponent(c1);
    state.AddSelectedComponent(c2);

    EXPECT_EQ(state.GetSelectedComponents().size(), 2u);
}

TEST(EditorSelectionStateTests, RemoveSelectedComponent)
{
    EditorSelectionState state;
    const ObjectId c1 = ObjectId::Generate();
    const ObjectId c2 = ObjectId::Generate();

    state.SetSelectedComponent(c1);
    state.AddSelectedComponent(c2);
    state.RemoveSelectedComponent(c1);

    EXPECT_FALSE(state.IsComponentSelected(c1));
    EXPECT_TRUE(state.IsComponentSelected(c2));
}

// --- NotifyObjectDestroyed ---

TEST(EditorSelectionStateTests, NotifyObjectDestroyedRemovesFromGameObjects)
{
    EditorSelectionState state;
    const ObjectId go = ObjectId::Generate();
    state.SetSelectedGameObject(go);

    state.NotifyObjectDestroyed(go);
    EXPECT_FALSE(state.HasGameObjectSelection());
}

TEST(EditorSelectionStateTests, NotifyObjectDestroyedRemovesFromComponents)
{
    EditorSelectionState state;
    const ObjectId co = ObjectId::Generate();
    state.SetSelectedComponent(co);

    state.NotifyObjectDestroyed(co);
    EXPECT_FALSE(state.HasComponentSelection());
}

TEST(EditorSelectionStateTests, NotifyObjectDestroyedIgnoresUnrelatedId)
{
    EditorSelectionState state;
    const ObjectId go = ObjectId::Generate();
    state.SetSelectedGameObject(go);

    state.NotifyObjectDestroyed(ObjectId::Generate());
    EXPECT_TRUE(state.HasGameObjectSelection());
    EXPECT_TRUE(state.IsGameObjectSelected(go));
}

TEST(EditorSelectionStateTests, NotifyObjectDestroyedNullIsNoOp)
{
    EditorSelectionState state;
    state.SetSelectedGameObject(ObjectId::Generate());
    state.NotifyObjectDestroyed(ObjectId::Null());
    EXPECT_TRUE(state.HasGameObjectSelection());
}
