#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/DScene.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/GC/DObjectRegistry.h"
#include "Runtime/Core/GC/GCManager.h"
#include "Runtime/Core/GC/WeakDObjectPtr.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Test/TestComponent.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{

void DestroyWorld(DWorld* world)
{
    if (world)
        GetReflectionRegistry().DestroyObject(world);
}

void DestroyScene(DScene* scene)
{
    if (scene)
        GetReflectionRegistry().DestroyObject(scene);
}

void DisposeStandaloneGameObject(GameObject* go)
{
    if (go)
        GetReflectionRegistry().DestroyObject(go);
}

void CollectAllGarbage()
{
    GetGCManager().CollectGarbage();
}

}

TEST(CoreObjectLifecycle, DWorld_Clear_RemovesRegisteredGameObjects)
{
    DWorld* world = DWorld::CreateWorld();
    ASSERT_NE(world, nullptr);
    GetDObjectRegistry().AddRoot(world->GetGCHandle());

    GameObject* go = world->CreateGameObject("Probe");
    ASSERT_NE(go, nullptr);
    ASSERT_EQ(world->GetGameObjects().size(), 1u);

    world->Clear();

    EXPECT_TRUE(world->GetGameObjects().empty());

    CollectAllGarbage();
    GetDObjectRegistry().RemoveRoot(world->GetGCHandle());
    DestroyWorld(world);
}

TEST(CoreObjectLifecycle, DScene_AddGameObject_DuplicateIgnoredKeepsSingleEntry)
{
    DScene* scene = CreateDObject<DScene>();
    ASSERT_NE(scene, nullptr);

    DWorld* world = DWorld::CreateWorld();
    ASSERT_NE(world, nullptr);

    GameObject* go = world->CreateGameObject("DupTest");
    ASSERT_NE(go, nullptr);

    scene->AddGameObject(go);
    scene->AddGameObject(go);

    ASSERT_EQ(scene->GetGameObjects().size(), 1u);

    scene->RemoveGameObject(go);
    DestroyWorld(world);
    DestroyScene(scene);
}

TEST(CoreObjectLifecycle, DWorld_CreateGameObjectInScene_WithGameObjectClass_AddsToSceneAndWorld)
{
    DScene* scene = CreateDObject<DScene>();
    ASSERT_NE(scene, nullptr);

    DWorld* world = DWorld::CreateWorld();
    ASSERT_NE(world, nullptr);

    DClass* goClass = GetReflectionRegistry().FindClassByName("GameObject");
    ASSERT_NE(goClass, nullptr);

    GameObject* go = world->CreateGameObjectInScene(scene, goClass);
    ASSERT_NE(go, nullptr);

    EXPECT_EQ(go->GetCurrentWorld(), world);
    ASSERT_EQ(scene->GetGameObjects().size(), 1u);
    EXPECT_EQ(scene->GetGameObjects()[0], go);
    ASSERT_EQ(world->GetGameObjects().size(), 1u);

    scene->RemoveGameObject(go);
    DestroyWorld(world);
    DestroyScene(scene);
}

TEST(CoreObjectLifecycle, DWorld_CreateGameObjectInScene_WithNullClass_ReturnsNull)
{
    DScene* scene = CreateDObject<DScene>();
    DWorld* world = DWorld::CreateWorld();

    EXPECT_EQ(world->CreateGameObjectInScene(scene, nullptr), nullptr);

    DestroyWorld(world);
    DestroyScene(scene);
}

TEST(CoreObjectLifecycle, DWorld_AddGameObjectFromScene_WithNull_NoOpDoesNotCrash)
{
    DWorld* world = DWorld::CreateWorld();
    ASSERT_NE(world, nullptr);

    world->AddGameObjectFromScene(nullptr);

    EXPECT_TRUE(world->GetGameObjects().empty());

    DestroyWorld(world);
}

TEST(CoreObjectLifecycle, DWorld_DestroyGameObject_WithNull_NoOpDoesNotCrash)
{
    DWorld* world = DWorld::CreateWorld();
    ASSERT_NE(world, nullptr);

    world->DestroyGameObject(nullptr);

    DestroyWorld(world);
}

TEST(CoreObjectLifecycle, DWorld_DestroyGameObject_UnlinkedUntilGCRedeems)
{
    DWorld* world = DWorld::CreateWorld();
    ASSERT_NE(world, nullptr);
    GetDObjectRegistry().AddRoot(world->GetGCHandle());

    GameObject* go = world->CreateGameObject("GCProbe");
    ASSERT_NE(go, nullptr);
    ASSERT_NE(go->AddComponent<TestComponent>(), nullptr);

    const size_t liveBefore = GetDObjectRegistry().GetLiveCount();
    WeakDObjectPtr<GameObject> weakGo(go);

    world->DestroyGameObject(go);

    EXPECT_TRUE(world->GetGameObjects().empty());
    EXPECT_EQ(GetDObjectRegistry().GetLiveCount(), liveBefore);
    EXPECT_TRUE(weakGo.IsValid());

    CollectAllGarbage();

    EXPECT_LT(GetDObjectRegistry().GetLiveCount(), liveBefore);
    EXPECT_FALSE(weakGo.IsValid());

    GetDObjectRegistry().RemoveRoot(world->GetGCHandle());
    DestroyWorld(world);
}

TEST(CoreObjectLifecycle, DScene_RemoveGameObject_NotTracked_NoOpLeavesEmptyList)
{
    DScene* scene = CreateDObject<DScene>();
    ASSERT_NE(scene, nullptr);

    GameObject* orphan = CreateDObject<GameObject>();
    ASSERT_NE(orphan, nullptr);

    scene->RemoveGameObject(orphan);

    EXPECT_TRUE(scene->GetGameObjects().empty());

    DestroyScene(scene);
    DisposeStandaloneGameObject(orphan);
}
