#include "Runtime/Core/DObject.h"
#include "Runtime/Core/GC/DObjectGCTypes.h"
#include "Runtime/Core/GC/DObjectRegistry.h"
#include "Runtime/Core/GC/GCManager.h"
#include "Runtime/Core/GC/WeakDObjectPtr.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Test/GCLifecycleTestComponent.h"
#include "Runtime/Test/SnapshotTestTypes.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{
class GCSweepTests : public ::testing::Test
{
protected:
    void SetUp() override { GCLifecycleTestComponent::Reset(); }

    void TearDown() override
    {
        // Drain anything left pending so global registry state is clean.
        GCLifecycleTestComponent::s_readyForFinishDestroy = true;
        GCManager& gc = GetGCManager();
        while (gc.HasPendingDestroy())
            gc.Tick();
        GCLifecycleTestComponent::Reset();
    }
};
}

TEST_F(GCSweepTests, CollectGarbageReclaimsUnreachable)
{
    auto& registry = GetReflectionRegistry();

    auto* root       = registry.CreateObject<DSnapshotTestComponentA>("DSnapshotTestComponentA");
    auto* referenced = registry.CreateObject<DSnapshotTestComponentB>("DSnapshotTestComponentB");
    auto* garbage    = registry.CreateObject<DSnapshotTestComponentB>("DSnapshotTestComponentB");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(referenced, nullptr);
    ASSERT_NE(garbage, nullptr);

    root->m_sibling = referenced;

    WeakDObjectPtr<DSnapshotTestComponentA> rootWeak(root);
    WeakDObjectPtr<DSnapshotTestComponentB> referencedWeak(referenced);
    WeakDObjectPtr<DSnapshotTestComponentB> garbageWeak(garbage);

    GetDObjectRegistry().AddRoot(root->GetGCHandle());

    GetGCManager().CollectGarbage();

    EXPECT_TRUE(rootWeak.IsValid());
    EXPECT_TRUE(referencedWeak.IsValid());
    EXPECT_FALSE(garbageWeak.IsValid());
    EXPECT_EQ(GetGCManager().GetState(), EGCState::Idle);
    EXPECT_FALSE(GetGCManager().HasPendingDestroy());

    GetDObjectRegistry().RemoveRoot(root->GetGCHandle());
    registry.DestroyObject(root);
    registry.DestroyObject(referenced);
}

TEST_F(GCSweepTests, WeakPointerNullsAtSweepStart)
{
    auto& registry = GetReflectionRegistry();

    auto* obj = registry.CreateObject<GCLifecycleTestComponent>("GCLifecycleTestComponent");
    ASSERT_NE(obj, nullptr);

    WeakDObjectPtr<GCLifecycleTestComponent> weak(obj);
    EXPECT_TRUE(weak.IsValid());

    GCLifecycleTestComponent::s_readyForFinishDestroy = false;

    GetGCManager().RequestCollect();
    GetGCManager().Tick();

    EXPECT_EQ(GetGCManager().GetState(), EGCState::Sweeping);
    EXPECT_TRUE(GetGCManager().HasPendingDestroy());
    EXPECT_FALSE(weak.IsValid());
    EXPECT_EQ(GCLifecycleTestComponent::s_beginDestroyCount, 1);
    EXPECT_EQ(GCLifecycleTestComponent::s_finishDestroyCount, 0);

    GCLifecycleTestComponent::s_readyForFinishDestroy = true;
    GetGCManager().Tick();

    EXPECT_EQ(GetGCManager().GetState(), EGCState::Idle);
    EXPECT_FALSE(GetGCManager().HasPendingDestroy());
    EXPECT_EQ(GCLifecycleTestComponent::s_finishDestroyCount, 1);
}

TEST_F(GCSweepTests, NewMarkBlockedUntilPendingEmpty)
{
    auto& registry = GetReflectionRegistry();

    auto* stuck = registry.CreateObject<GCLifecycleTestComponent>("GCLifecycleTestComponent");
    ASSERT_NE(stuck, nullptr);

    GCLifecycleTestComponent::s_readyForFinishDestroy = false;
    GetGCManager().RequestCollect();
    GetGCManager().Tick();
    ASSERT_TRUE(GetGCManager().HasPendingDestroy());

    auto* late = registry.CreateObject<DSnapshotTestComponentB>("DSnapshotTestComponentB");
    ASSERT_NE(late, nullptr);
    WeakDObjectPtr<DSnapshotTestComponentB> lateWeak(late);

    // A collect cannot start while objects are pending, so `late` survives.
    GetGCManager().RequestCollect();
    GetGCManager().Tick();
    EXPECT_TRUE(lateWeak.IsValid());
    EXPECT_TRUE(GetGCManager().HasPendingDestroy());

    GCLifecycleTestComponent::s_readyForFinishDestroy = true;
    GetGCManager().Tick();
    ASSERT_FALSE(GetGCManager().HasPendingDestroy());
    EXPECT_EQ(GetGCManager().GetState(), EGCState::Idle);

    // Now a fresh collect is allowed and reclaims the still-unrooted `late`.
    GetGCManager().RequestCollect();
    GetGCManager().Tick();
    EXPECT_FALSE(lateWeak.IsValid());
}

TEST_F(GCSweepTests, PendingDestroyPersistsAcrossTicks)
{
    auto& registry = GetReflectionRegistry();

    auto* obj = registry.CreateObject<GCLifecycleTestComponent>("GCLifecycleTestComponent");
    ASSERT_NE(obj, nullptr);

    GCLifecycleTestComponent::s_readyForFinishDestroy = false;
    GetGCManager().RequestCollect();
    GetGCManager().Tick();

    for (int i = 0; i < 5; ++i)
    {
        GetGCManager().Tick();
        EXPECT_TRUE(GetGCManager().HasPendingDestroy());
        EXPECT_EQ(GetGCManager().GetState(), EGCState::Sweeping);
    }

    // BeginDestroy runs exactly once even though the object lingers for many frames.
    EXPECT_EQ(GCLifecycleTestComponent::s_beginDestroyCount, 1);
    EXPECT_EQ(GCLifecycleTestComponent::s_finishDestroyCount, 0);

    GCLifecycleTestComponent::s_readyForFinishDestroy = true;
    GetGCManager().Tick();
    EXPECT_FALSE(GetGCManager().HasPendingDestroy());
    EXPECT_EQ(GCLifecycleTestComponent::s_finishDestroyCount, 1);
}

TEST_F(GCSweepTests, FullCycleInvokesLifecycleHooks)
{
    auto& registry = GetReflectionRegistry();

    auto* obj = registry.CreateObject<GCLifecycleTestComponent>("GCLifecycleTestComponent");
    ASSERT_NE(obj, nullptr);
    WeakDObjectPtr<GCLifecycleTestComponent> weak(obj);

    GetGCManager().CollectGarbage();

    EXPECT_EQ(GCLifecycleTestComponent::s_beginDestroyCount, 1);
    EXPECT_EQ(GCLifecycleTestComponent::s_finishDestroyCount, 1);
    EXPECT_FALSE(weak.IsValid());
    EXPECT_EQ(GetGCManager().GetState(), EGCState::Idle);
    EXPECT_FALSE(GetGCManager().HasPendingDestroy());
}
