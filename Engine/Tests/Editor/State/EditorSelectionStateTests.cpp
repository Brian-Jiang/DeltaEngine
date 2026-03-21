#include "Editor/EditorSelectionState.h"
#include "Runtime/Core/DComponent.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(EditorSelectionStateTests, SelectGameObjectClearsComponentAndAssetSelections)
{
    DWorld world;
    GameObject* gameObject = world.CreateGameObject("Actor");
    DComponent* component = gameObject->AddComponent<DComponent>("Component");
    EditorSelectionState state;

    state.SelectAsset(AssetId::Generate());
    state.SelectComponent(component);
    state.SelectGameObject(gameObject);

    ASSERT_EQ(state.GetSelectedGameObjects().size(), 1u);
    EXPECT_EQ(state.GetSelectedGameObjects().front(), gameObject);
    EXPECT_TRUE(state.GetSelectedComponents().empty());
    EXPECT_FALSE(state.HasAssetSelection());
    EXPECT_TRUE(state.HasSelection());
}

TEST(EditorSelectionStateTests, AdditiveSelectionPreservesEntriesAndClearsAssetSelection)
{
    DWorld world;
    GameObject* firstObject = world.CreateGameObject("First");
    GameObject* secondObject = world.CreateGameObject("Second");
    DComponent* component = secondObject->AddComponent<DComponent>("Component");
    EditorSelectionState state;

    state.SelectAsset(AssetId::Generate());
    state.AddGameObjectToSelection(firstObject);
    state.AddGameObjectToSelection(secondObject);
    state.AddComponentToSelection(component);

    EXPECT_FALSE(state.HasAssetSelection());
    ASSERT_EQ(state.GetSelectedGameObjects().size(), 2u);
    EXPECT_EQ(state.GetSelectedGameObjects()[0], firstObject);
    EXPECT_EQ(state.GetSelectedGameObjects()[1], secondObject);
    ASSERT_EQ(state.GetSelectedComponents().size(), 1u);
    EXPECT_EQ(state.GetSelectedComponents().front(), component);
}

TEST(EditorSelectionStateTests, ContextGameObjectPrefersSelectedGameObjectThenComponentOwner)
{
    DWorld world;
    GameObject* selectedObject = world.CreateGameObject("SelectedObject");
    GameObject* ownerObject = world.CreateGameObject("OwnerObject");
    DComponent* component = ownerObject->AddComponent<DComponent>("Component");

    EditorSelectionState mixedSelection;
    mixedSelection.AddGameObjectToSelection(selectedObject);
    mixedSelection.AddComponentToSelection(component);
    EXPECT_EQ(mixedSelection.GetContextGameObject(), selectedObject);

    EditorSelectionState componentOnlySelection;
    componentOnlySelection.SelectComponent(component);
    EXPECT_EQ(componentOnlySelection.GetContextGameObject(), ownerObject);
}

TEST(EditorSelectionStateTests, HasSelectionTracksEmptyObjectComponentAndAssetStates)
{
    DWorld world;
    GameObject* gameObject = world.CreateGameObject("Actor");
    DComponent* component = gameObject->AddComponent<DComponent>("Component");
    EditorSelectionState state;

    EXPECT_FALSE(state.HasSelection());

    state.SelectAsset(AssetId::Generate());
    EXPECT_TRUE(state.HasSelection());

    state.ClearSelection();
    EXPECT_FALSE(state.HasSelection());

    state.SelectGameObject(gameObject);
    EXPECT_TRUE(state.HasSelection());

    state.ClearSelection();
    state.SelectComponent(component);
    EXPECT_TRUE(state.HasSelection());
}
