#include "Editor/EditorSelectionState.h"

#include "Runtime/Core/Delegates/MulticastDelegate.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

// --- OnSelectionChanged ---

TEST(EditorSelectionStateTests, OnSelectionChanged_FiresOnSetSelectedGameObject)
{
    EditorSelectionState state;
    int callCount = 0;
    state.OnSelectionChanged.AddLambda([&callCount]() { ++callCount; });

    state.SetSelectedGameObject(ObjectId::Generate());
    EXPECT_EQ(callCount, 1);
}

TEST(EditorSelectionStateTests, OnSelectionChanged_SingleBroadcastOnSetSelectedGameObject)
{
    EditorSelectionState state;
    int callCount = 0;
    state.OnSelectionChanged.AddLambda([&callCount]() { ++callCount; });

    state.SetSelectedAsset(AssetId::Generate());
    callCount = 0;

    state.SetSelectedGameObject(ObjectId::Generate());
    EXPECT_EQ(callCount, 1);
}

TEST(EditorSelectionStateTests, OnSelectionChanged_FiresOnAddSelectedComponent)
{
    EditorSelectionState state;
    int callCount = 0;
    state.OnSelectionChanged.AddLambda([&callCount]() { ++callCount; });

    state.AddSelectedComponent(ObjectId::Generate());
    EXPECT_EQ(callCount, 1);
}

TEST(EditorSelectionStateTests, OnSelectionChanged_FiresOnSetSelectedAsset)
{
    EditorSelectionState state;
    int callCount = 0;
    state.OnSelectionChanged.AddLambda([&callCount]() { ++callCount; });

    state.SetSelectedAsset(AssetId::Generate());
    EXPECT_EQ(callCount, 1);
}

TEST(EditorSelectionStateTests, OnSelectionChanged_FiresOnSetSelectedFolder)
{
    EditorSelectionState state;
    int callCount = 0;
    state.OnSelectionChanged.AddLambda([&callCount]() { ++callCount; });

    state.SetSelectedFolder("Materials");
    EXPECT_EQ(callCount, 1);
}

TEST(EditorSelectionStateTests, OnSelectionChanged_FiresOnClearAll)
{
    EditorSelectionState state;
    int callCount = 0;
    state.OnSelectionChanged.AddLambda([&callCount]() { ++callCount; });

    state.SetSelectedGameObject(ObjectId::Generate());
    state.SetSelectedComponent(ObjectId::Generate());
    callCount = 0;

    state.ClearAll();
    EXPECT_EQ(callCount, 1);
}

TEST(EditorSelectionStateTests, OnSelectionChanged_FiresOnCrossDomainIndirectClear)
{
    EditorSelectionState state;
    int callCount = 0;
    state.OnSelectionChanged.AddLambda([&callCount]() { ++callCount; });

    state.SetSelectedGameObject(ObjectId::Generate());
    callCount = 0;

    state.SetSelectedAsset(AssetId::Generate());
    EXPECT_EQ(callCount, 1);
}

TEST(EditorSelectionStateTests, OnSelectionChanged_DoesNotFireOnDuplicateAdd)
{
    EditorSelectionState state;
    const ObjectId go = ObjectId::Generate();
    int callCount = 0;
    state.OnSelectionChanged.AddLambda([&callCount]() { ++callCount; });

    state.SetSelectedGameObject(go);
    callCount = 0;

    state.AddSelectedGameObject(go);
    EXPECT_EQ(callCount, 0);
}

TEST(EditorSelectionStateTests, OnSelectionChanged_DoesNotFireOnNoOpRemove)
{
    EditorSelectionState state;
    int callCount = 0;
    state.OnSelectionChanged.AddLambda([&callCount]() { ++callCount; });

    state.SetSelectedGameObject(ObjectId::Generate());
    callCount = 0;

    state.RemoveSelectedGameObject(ObjectId::Generate());
    EXPECT_EQ(callCount, 0);
}

TEST(EditorSelectionStateTests, OnSelectionChanged_FiresOnNotifyObjectDestroyedSelectedId)
{
    EditorSelectionState state;
    const ObjectId go = ObjectId::Generate();
    int callCount = 0;
    state.OnSelectionChanged.AddLambda([&callCount]() { ++callCount; });

    state.SetSelectedGameObject(go);
    callCount = 0;

    state.NotifyObjectDestroyed(go);
    EXPECT_EQ(callCount, 1);
    EXPECT_FALSE(state.HasGameObjectSelection());
}

TEST(EditorSelectionStateTests, OnSelectionChanged_DoesNotFireOnUnrelatedNotifyObjectDestroyed)
{
    EditorSelectionState state;
    int callCount = 0;
    state.OnSelectionChanged.AddLambda([&callCount]() { ++callCount; });

    state.SetSelectedGameObject(ObjectId::Generate());
    callCount = 0;

    state.NotifyObjectDestroyed(ObjectId::Generate());
    EXPECT_EQ(callCount, 0);
}

TEST(EditorSelectionStateTests, OnSelectionChanged_DoesNotFireOnNullNotifyObjectDestroyed)
{
    EditorSelectionState state;
    int callCount = 0;
    state.OnSelectionChanged.AddLambda([&callCount]() { ++callCount; });

    state.SetSelectedGameObject(ObjectId::Generate());
    callCount = 0;

    state.NotifyObjectDestroyed(ObjectId::Null());
    EXPECT_EQ(callCount, 0);
}

TEST(EditorSelectionStateTests, OnSelectionChanged_MultiSelectPartialDestroy)
{
    EditorSelectionState state;
    const ObjectId go1 = ObjectId::Generate();
    const ObjectId go2 = ObjectId::Generate();
    int callCount = 0;
    state.OnSelectionChanged.AddLambda([&callCount]() { ++callCount; });

    state.SetSelectedGameObject(go1);
    state.AddSelectedGameObject(go2);
    callCount = 0;

    state.NotifyObjectDestroyed(go1);
    EXPECT_EQ(callCount, 1);
    EXPECT_FALSE(state.IsGameObjectSelected(go1));
    EXPECT_TRUE(state.IsGameObjectSelected(go2));
}

TEST(EditorSelectionStateTests, OnSelectionChanged_MultipleSubscribersInRegistrationOrder)
{
    EditorSelectionState state;
    std::vector<int> callOrder;

    state.OnSelectionChanged.AddLambda([&callOrder]() { callOrder.push_back(1); });
    state.OnSelectionChanged.AddLambda([&callOrder]() { callOrder.push_back(2); });
    state.OnSelectionChanged.AddLambda([&callOrder]() { callOrder.push_back(3); });

    state.SetSelectedGameObject(ObjectId::Generate());

    ASSERT_EQ(callOrder.size(), 3u);
    EXPECT_EQ(callOrder[0], 1);
    EXPECT_EQ(callOrder[1], 2);
    EXPECT_EQ(callOrder[2], 3);
}

TEST(EditorSelectionStateTests, OnSelectionChanged_RemoveHandleStopsFutureBroadcasts)
{
    EditorSelectionState state;
    int counter = 0;

    state.OnSelectionChanged.AddLambda([&counter]() { ++counter; });
    const FDelegateHandle middleHandle = state.OnSelectionChanged.AddLambda([&counter]() { counter += 10; });
    state.OnSelectionChanged.AddLambda([&counter]() { counter += 100; });

    state.SetSelectedGameObject(ObjectId::Generate());
    EXPECT_EQ(counter, 111);

    EXPECT_TRUE(state.OnSelectionChanged.Remove(middleHandle));

    counter = 0;
    state.SetSelectedGameObject(ObjectId::Generate());
    EXPECT_EQ(counter, 101);
}

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

// --- Folder selection ---

TEST(EditorSelectionStateTests, SetSelectedFolderClearsAssets)
{
    EditorSelectionState state;
    state.SetSelectedAsset(AssetId::Generate());
    EXPECT_TRUE(state.HasAssetSelection());

    state.SetSelectedFolder("Materials");

    EXPECT_FALSE(state.HasAssetSelection());
    EXPECT_TRUE(state.HasFolderSelection());
    EXPECT_TRUE(state.IsFolderSelected("Materials"));
}

TEST(EditorSelectionStateTests, SetSelectedFolderClearsObjectsAndComponents)
{
    EditorSelectionState state;
    state.SetSelectedGameObject(ObjectId::Generate());
    state.SetSelectedComponent(ObjectId::Generate());

    state.SetSelectedFolder("Materials");

    EXPECT_FALSE(state.HasGameObjectSelection());
    EXPECT_FALSE(state.HasComponentSelection());
    EXPECT_TRUE(state.HasFolderSelection());
}

TEST(EditorSelectionStateTests, SetSelectedAssetClearsFolder)
{
    EditorSelectionState state;
    state.SetSelectedFolder("Materials");

    state.SetSelectedAsset(AssetId::Generate());

    EXPECT_FALSE(state.HasFolderSelection());
    EXPECT_TRUE(state.HasAssetSelection());
}

// --- Component selection ---

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

TEST(EditorSelectionStateTests, GameObjectSelectionClearsComponents)
{
    EditorSelectionState state;
    const ObjectId co = ObjectId::Generate();

    state.SetSelectedComponent(co);
    state.SetSelectedGameObject(ObjectId::Generate());

    EXPECT_FALSE(state.HasComponentSelection());
    EXPECT_FALSE(state.IsComponentSelected(co));
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
