#include "Runtime/Core/GC/DObjectGCTypes.h"
#include "Runtime/Core/GC/DObjectRegistry.h"
#include "Runtime/Core/GC/StrongDObjectPtr.h"
#include "Runtime/Core/GC/WeakDObjectPtr.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Test/TestComponent.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(GCObjectRegistryTests, CreatedObjectHasResolvingHandle)
{
    auto& registry = GetReflectionRegistry();
    TestComponent* object = registry.CreateObject<TestComponent>("TestComponent");
    ASSERT_NE(object, nullptr);

    DObjectHandle handle = object->GetGCHandle();
    EXPECT_TRUE(handle.IsSet());
    EXPECT_TRUE(GetDObjectRegistry().IsValid(handle));
    EXPECT_EQ(GetDObjectRegistry().Resolve(handle), object);

    registry.DestroyObject(object);
}

TEST(GCObjectRegistryTests, SlotReuseBumpsVersion)
{
    auto& registry = GetReflectionRegistry();

    TestComponent* first = registry.CreateObject<TestComponent>("TestComponent");
    ASSERT_NE(first, nullptr);
    DObjectHandle firstHandle = first->GetGCHandle();

    registry.DestroyObject(first);
    EXPECT_FALSE(GetDObjectRegistry().IsValid(firstHandle));

    TestComponent* second = registry.CreateObject<TestComponent>("TestComponent");
    ASSERT_NE(second, nullptr);
    DObjectHandle secondHandle = second->GetGCHandle();

    EXPECT_EQ(secondHandle.m_slotIndex, firstHandle.m_slotIndex);
    EXPECT_NE(secondHandle.m_version, firstHandle.m_version);
    EXPECT_FALSE(GetDObjectRegistry().IsValid(firstHandle));
    EXPECT_TRUE(GetDObjectRegistry().IsValid(secondHandle));

    registry.DestroyObject(second);
}

TEST(GCObjectRegistryTests, WeakPointerNullsOnFree)
{
    auto& registry = GetReflectionRegistry();
    TestComponent* object = registry.CreateObject<TestComponent>("TestComponent");
    ASSERT_NE(object, nullptr);

    WeakDObjectPtr<TestComponent> weak(object);
    EXPECT_TRUE(weak.IsValid());
    EXPECT_EQ(weak.Get(), object);

    registry.DestroyObject(object);

    EXPECT_FALSE(weak.IsValid());
    EXPECT_EQ(weak.Get(), nullptr);
}

TEST(GCObjectRegistryTests, StrongPointerRegistersRoot)
{
    auto& registry = GetReflectionRegistry();
    TestComponent* object = registry.CreateObject<TestComponent>("TestComponent");
    ASSERT_NE(object, nullptr);
    DObjectHandle handle = object->GetGCHandle();

    {
        StrongDObjectPtr<TestComponent> strong(object);
        EXPECT_EQ(strong.Get(), object);

        auto roots = GetDObjectRegistry().GetRoots();
        bool found = false;
        for (const DObjectHandle& root : roots)
            found = found || (root == handle);
        EXPECT_TRUE(found);
    }

    auto rootsAfter = GetDObjectRegistry().GetRoots();
    bool stillRoot = false;
    for (const DObjectHandle& root : rootsAfter)
        stillRoot = stillRoot || (root == handle);
    EXPECT_FALSE(stillRoot);

    registry.DestroyObject(object);
}

TEST(GCObjectRegistryTests, DestroyObjectDestroysImmediately)
{
    auto& registry = GetReflectionRegistry();
    const size_t before = GetDObjectRegistry().GetLiveCount();

    TestComponent* object = registry.CreateObject<TestComponent>("TestComponent");
    ASSERT_NE(object, nullptr);
    EXPECT_EQ(GetDObjectRegistry().GetLiveCount(), before + 1);

    registry.DestroyObject(object);
    EXPECT_EQ(GetDObjectRegistry().GetLiveCount(), before);
}
