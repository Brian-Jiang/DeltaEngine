#include "Runtime/Core/Delegates/Delegate.h"
#include "Runtime/Core/Delegates/MulticastDelegate.h"
#include "Runtime/Core/GC/DObjectRegistry.h"
#include "Runtime/Core/GC/GCManager.h"
#include "Runtime/Core/GC/WeakDObjectPtr.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Test/GCLifecycleTestComponent.h"
#include "Runtime/Test/TestComponent.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{

class DelegateGCBindingTests : public ::testing::Test
{
protected:
    void SetUp() override { GCLifecycleTestComponent::Reset(); }

    void TearDown() override
    {
        GCLifecycleTestComponent::s_readyForFinishDestroy = true;
        GetGCManager().DrainPendingDestroyWithTimeout();
        GCLifecycleTestComponent::Reset();
    }
};

}

TEST_F(DelegateGCBindingTests, BindDObject_InvokesWhileAlive)
{
    auto& registry = GetReflectionRegistry();
    TestComponent* object = registry.CreateObject<TestComponent>("TestComponent");
    ASSERT_NE(object, nullptr);

    TDelegate<int(int, int)> delegate;
    delegate.BindDObject(object, &TestComponent::TestAdd);

    EXPECT_TRUE(delegate.IsBound());
    EXPECT_EQ(delegate.Execute(2, 3), 5);

    registry.DestroyObject(object);
}

TEST_F(DelegateGCBindingTests, BindDObject_AfterDestroy_IsUnbound)
{
    auto& registry = GetReflectionRegistry();
    TestComponent* object = registry.CreateObject<TestComponent>("TestComponent");
    ASSERT_NE(object, nullptr);

    TDelegate<int(int, int)> delegate;
    delegate.BindDObject(object, &TestComponent::TestAdd);

    registry.DestroyObject(object);

    EXPECT_FALSE(delegate.IsBound());
    EXPECT_EQ(delegate.ExecuteIfBound(2, 3), 0);
}

TEST_F(DelegateGCBindingTests, AddDObject_BroadcastsWhileAlive)
{
    auto& registry = GetReflectionRegistry();
    GCLifecycleTestComponent* object = registry.CreateObject<GCLifecycleTestComponent>("GCLifecycleTestComponent");
    ASSERT_NE(object, nullptr);

    TMulticastDelegate<void()> multicast;
    multicast.AddDObject(object, &GCLifecycleTestComponent::DelegatePing);

    multicast.Broadcast();

    EXPECT_EQ(GCLifecycleTestComponent::s_delegatePingCount, 1);

    registry.DestroyObject(object);
}

TEST_F(DelegateGCBindingTests, AddDObject_AfterDestroy_SkipsAndCompacts)
{
    auto& registry = GetReflectionRegistry();
    GCLifecycleTestComponent* object = registry.CreateObject<GCLifecycleTestComponent>("GCLifecycleTestComponent");
    ASSERT_NE(object, nullptr);

    TMulticastDelegate<void()> multicast;
    multicast.AddDObject(object, &GCLifecycleTestComponent::DelegatePing);

    int lambdaCount = 0;
    multicast.AddLambda([&lambdaCount]() { ++lambdaCount; });

    registry.DestroyObject(object);

    GCLifecycleTestComponent::s_delegatePingCount = 0;
    lambdaCount = 0;

    multicast.Broadcast();

    EXPECT_EQ(GCLifecycleTestComponent::s_delegatePingCount, 0);
    EXPECT_EQ(lambdaCount, 1);

    lambdaCount = 0;
    multicast.Broadcast();
    EXPECT_EQ(lambdaCount, 1);
    EXPECT_TRUE(multicast.IsBound());
}

TEST_F(DelegateGCBindingTests, AddDObject_RemoveAll_LiveObject)
{
    auto& registry = GetReflectionRegistry();
    GCLifecycleTestComponent* objectA = registry.CreateObject<GCLifecycleTestComponent>("GCLifecycleTestComponent");
    GCLifecycleTestComponent* objectB = registry.CreateObject<GCLifecycleTestComponent>("GCLifecycleTestComponent");
    ASSERT_NE(objectA, nullptr);
    ASSERT_NE(objectB, nullptr);

    TMulticastDelegate<void()> multicast;
    multicast.AddDObject(objectA, &GCLifecycleTestComponent::DelegatePing);
    multicast.AddDObject(objectB, &GCLifecycleTestComponent::DelegatePing);

    EXPECT_EQ(multicast.RemoveAll(objectA), 1u);

    GCLifecycleTestComponent::s_delegatePingCount = 0;
    multicast.Broadcast();

    EXPECT_EQ(GCLifecycleTestComponent::s_delegatePingCount, 1);

    registry.DestroyObject(objectA);
    registry.DestroyObject(objectB);
}

TEST_F(DelegateGCBindingTests, AddDObject_PendingKill_SkipsAtSweepStart)
{
    auto& registry = GetReflectionRegistry();
    GCLifecycleTestComponent* object = registry.CreateObject<GCLifecycleTestComponent>("GCLifecycleTestComponent");
    ASSERT_NE(object, nullptr);

    TMulticastDelegate<void()> multicast;
    multicast.AddDObject(object, &GCLifecycleTestComponent::DelegatePing);

    WeakDObjectPtr<GCLifecycleTestComponent> weak(object);
    EXPECT_TRUE(weak.IsValid());

    GCLifecycleTestComponent::s_readyForFinishDestroy = false;

    GetGCManager().RequestCollect();
    GetGCManager().Tick();

    EXPECT_EQ(GetGCManager().GetState(), EGCState::Sweeping);
    EXPECT_TRUE(GetGCManager().HasPendingDestroy());
    EXPECT_FALSE(weak.IsValid());

    GCLifecycleTestComponent::s_delegatePingCount = 0;
    multicast.Broadcast();
    EXPECT_EQ(GCLifecycleTestComponent::s_delegatePingCount, 0);

    GCLifecycleTestComponent::s_readyForFinishDestroy = true;
    GetGCManager().Tick();
}
