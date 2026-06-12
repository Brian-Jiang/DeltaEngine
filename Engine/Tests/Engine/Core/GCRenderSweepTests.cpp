#include "Runtime/Core/GC/DObjectGCTypes.h"
#include "Runtime/Core/GC/DObjectRegistry.h"
#include "Runtime/Core/GC/GCManager.h"
#include "Runtime/Graphics/RenderResourceReleaseQueue.h"
#include "Runtime/Graphics/RenderResourceReleaseService.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Test/GCRenderLifecycleTestRenderer.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{
class GCRenderSweepTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        GCRenderLifecycleTestRenderer::Reset();
        m_completedFence = 0;
        m_queue.SetFenceCompleteChecker([this](uint64_t fenceValue)
        {
            return fenceValue <= m_completedFence;
        });
        m_queue.SetLastSubmittedFence(1);
        GetRenderResourceReleaseService().RegisterQueue(&m_queue);
    }

    void TearDown() override
    {
        m_completedFence = UINT64_MAX;
        m_queue.ProcessCompleted();
        GetGCManager().DrainPendingDestroyWithTimeout();
        GetRenderResourceReleaseService().UnregisterQueue(&m_queue);
        GCRenderLifecycleTestRenderer::Reset();
    }

    uint64_t                   m_completedFence = 0;
    RenderResourceReleaseQueue m_queue;
};
}

TEST_F(GCRenderSweepTests, BeginDestroyDetachesProxyImmediately)
{
    auto& registry = GetReflectionRegistry();

    auto* renderer = registry.CreateObject<GCRenderLifecycleTestRenderer>("GCRenderLifecycleTestRenderer");
    ASSERT_NE(renderer, nullptr);
    renderer->CreateRenderProxy();

    GetGCManager().RequestCollect();
    GetGCManager().Tick();

    EXPECT_EQ(GCRenderLifecycleTestRenderer::s_beginDestroyCount, 1);
    EXPECT_EQ(GetGCManager().GetState(), EGCState::Sweeping);
    EXPECT_TRUE(GetGCManager().HasPendingDestroy());
    EXPECT_EQ(m_queue.GetPendingCount(), 1u);

    m_completedFence = 1;
    m_queue.ProcessCompleted();
    GetGCManager().Tick();
    EXPECT_EQ(GCRenderLifecycleTestRenderer::s_finishDestroyCount, 1);
}

TEST_F(GCRenderSweepTests, DeferredExclusiveProxyBlocksFinishDestroyUntilFenceCompletes)
{
    auto& registry = GetReflectionRegistry();

    auto* renderer = registry.CreateObject<GCRenderLifecycleTestRenderer>("GCRenderLifecycleTestRenderer");
    ASSERT_NE(renderer, nullptr);
    renderer->CreateRenderProxy();
    GCRenderLifecycleTestRenderer::s_proxyHasExclusiveResources = true;

    GetDObjectRegistry().AddRoot(renderer->GetGCHandle());
    GetDObjectRegistry().RemoveRoot(renderer->GetGCHandle());

    GetGCManager().RequestCollect();
    GetGCManager().Tick();

    ASSERT_EQ(GetGCManager().GetState(), EGCState::Sweeping);
    EXPECT_EQ(GCRenderLifecycleTestRenderer::s_beginDestroyCount, 1);
    EXPECT_EQ(GCRenderLifecycleTestRenderer::s_finishDestroyCount, 0);

    GetGCManager().Tick();
    EXPECT_EQ(GCRenderLifecycleTestRenderer::s_finishDestroyCount, 0);

    m_completedFence = 1;
    m_queue.ProcessCompleted();

    GetGCManager().Tick();
    EXPECT_EQ(GCRenderLifecycleTestRenderer::s_finishDestroyCount, 1);
    EXPECT_EQ(GetGCManager().GetState(), EGCState::Idle);
    EXPECT_FALSE(GetGCManager().HasPendingDestroy());
}

TEST_F(GCRenderSweepTests, SharedOnlyProxyCompletesSweepImmediately)
{
    auto& registry = GetReflectionRegistry();

    auto* renderer = registry.CreateObject<GCRenderLifecycleTestRenderer>("GCRenderLifecycleTestRenderer");
    ASSERT_NE(renderer, nullptr);
    GCRenderLifecycleTestRenderer::s_proxyHasExclusiveResources = false;
    renderer->CreateRenderProxy();

    GetDObjectRegistry().AddRoot(renderer->GetGCHandle());
    GetDObjectRegistry().RemoveRoot(renderer->GetGCHandle());

    GetGCManager().CollectGarbage();

    EXPECT_EQ(GCRenderLifecycleTestRenderer::s_beginDestroyCount, 1);
    EXPECT_EQ(GCRenderLifecycleTestRenderer::s_finishDestroyCount, 1);
    EXPECT_EQ(GetGCManager().GetState(), EGCState::Idle);
}
