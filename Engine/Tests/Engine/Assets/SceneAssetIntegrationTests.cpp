#include "Runtime/Assets/PA_DScene.h"
#include "Runtime/Core/DScene.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(SceneAssetIntegrationTests, CreateGameObjectInSceneConnectsAssetSceneAndWorld)
{
    PA_DScene* asset = PA_DScene::Create("RuntimeScene");
    ASSERT_NE(asset, nullptr);
    EXPECT_EQ(asset->GetHeader().m_className, "PA_DScene");
    EXPECT_FALSE(asset->GetAssetId().IsNull());

    DScene* scene = asset->GetScene();
    ASSERT_NE(scene, nullptr);
    EXPECT_EQ(scene->GetOwningAsset(), asset);
    EXPECT_EQ(scene->GetName(), "RuntimeScene");

    DWorld* world = DWorld::CreateWorld();
    ASSERT_NE(world, nullptr);
    world->SetActiveScene(scene);
    EXPECT_EQ(world->GetActiveScene(), scene);

    GameObject* gameObject = world->CreateGameObjectInScene(world->GetActiveScene(), "PersistedActor");
    ASSERT_NE(gameObject, nullptr);
    EXPECT_EQ(gameObject->GetOwningAsset(), asset);
    EXPECT_EQ(gameObject->GetCurrentWorld(), world);
    EXPECT_EQ(gameObject->GetName(), "PersistedActor");
    EXPECT_FALSE(gameObject->GetObjectId().IsNull());
    EXPECT_EQ(asset->FindObject(gameObject->GetObjectId()), gameObject);

    ASSERT_EQ(scene->GetGameObjects().size(), 1u);
    EXPECT_EQ(scene->GetGameObjects().front(), gameObject);
    ASSERT_EQ(world->GetGameObjects().size(), 1u);
    EXPECT_EQ(world->GetGameObjects().front(), gameObject);
    EXPECT_TRUE(world->IsGameObjectsChanged());
}
