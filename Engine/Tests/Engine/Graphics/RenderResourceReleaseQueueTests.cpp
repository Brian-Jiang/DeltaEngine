#include "Runtime/Graphics/RenderResourceReleaseQueue.h"
#include "Runtime/Graphics/RenderResourceReleaseService.h"
#include "Runtime/Graphics/RenderProxy/RenderProxy.h"

#include <gtest/gtest.h>

#include <memory>

using namespace DeltaEngine;

namespace
{
class TestRenderProxy : public RenderProxy
{
public:
    explicit TestRenderProxy(bool hasExclusiveResources)
        : m_hasExclusiveResources(hasExclusiveResources)
    {
    }

    bool HasExclusiveGPUResources() const override { return m_hasExclusiveResources; }

    bool m_hasExclusiveResources = false;
};

class RenderResourceReleaseQueueTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_completedFence = 0;
        m_queue.SetFenceCompleteChecker([this](uint64_t fenceValue)
        {
            return fenceValue <= m_completedFence;
        });
        m_queue.SetLastSubmittedFence(10);
    }

    uint64_t                   m_completedFence = 0;
    RenderResourceReleaseQueue m_queue;
};
}

TEST_F(RenderResourceReleaseQueueTests, InvalidToken_IsAlwaysComplete)
{
    EXPECT_TRUE(m_queue.IsComplete(RenderResourceReleaseToken::Invalid()));
}

TEST_F(RenderResourceReleaseQueueTests, EnqueueWithIncompleteFence_StaysPendingUntilProcessed)
{
    auto proxy = std::make_shared<TestRenderProxy>(true);
    RenderResourceReleaseToken token = m_queue.Enqueue(proxy, 5);

    ASSERT_TRUE(token.IsValid());
    EXPECT_FALSE(m_queue.IsComplete(token));
    EXPECT_EQ(m_queue.GetPendingCount(), 1u);

    m_completedFence = 4;
    EXPECT_EQ(m_queue.ProcessCompleted(), 0u);
    EXPECT_FALSE(m_queue.IsComplete(token));

    m_completedFence = 5;
    EXPECT_EQ(m_queue.ProcessCompleted(), 1u);
    EXPECT_TRUE(m_queue.IsComplete(token));
    EXPECT_EQ(m_queue.GetPendingCount(), 0u);
}

TEST_F(RenderResourceReleaseQueueTests, EnqueueWithoutExplicitFence_UsesLastSubmittedFence)
{
    m_queue.SetLastSubmittedFence(7);

    auto proxy = std::make_shared<TestRenderProxy>(true);
    RenderResourceReleaseToken token = m_queue.Enqueue(proxy);

    m_completedFence = 6;
    EXPECT_EQ(m_queue.ProcessCompleted(), 0u);
    EXPECT_FALSE(m_queue.IsComplete(token));

    m_completedFence = 7;
    EXPECT_EQ(m_queue.ProcessCompleted(), 1u);
    EXPECT_TRUE(m_queue.IsComplete(token));
}

TEST_F(RenderResourceReleaseQueueTests, MultipleEntries_DrainIndependently)
{
    auto proxyA = std::make_shared<TestRenderProxy>(true);
    auto proxyB = std::make_shared<TestRenderProxy>(true);

    RenderResourceReleaseToken tokenA = m_queue.Enqueue(proxyA, 3);
    RenderResourceReleaseToken tokenB = m_queue.Enqueue(proxyB, 8);

    m_completedFence = 3;
    EXPECT_EQ(m_queue.ProcessCompleted(), 1u);
    EXPECT_TRUE(m_queue.IsComplete(tokenA));
    EXPECT_FALSE(m_queue.IsComplete(tokenB));

    m_completedFence = 8;
    EXPECT_EQ(m_queue.ProcessCompleted(), 1u);
    EXPECT_TRUE(m_queue.IsComplete(tokenB));
}

TEST(RenderResourceReleaseServiceTests, UnregisteredService_ReturnsInvalidTokenAndIsComplete)
{
    RenderResourceReleaseService& service = GetRenderResourceReleaseService();

    RenderResourceReleaseQueue queue;
    service.RegisterQueue(&queue);
    service.UnregisterQueue(&queue);

    auto proxy = std::make_shared<TestRenderProxy>(true);
    RenderResourceReleaseToken token = service.DeferRelease(proxy);

    EXPECT_FALSE(token.IsValid());
    EXPECT_TRUE(service.IsComplete(token));
}

TEST(RenderResourceReleaseServiceTests, RegisteredService_DelegatesToQueue)
{
    uint64_t completedFence = 0;
    RenderResourceReleaseQueue queue;
    queue.SetFenceCompleteChecker([&completedFence](uint64_t fenceValue)
    {
        return fenceValue <= completedFence;
    });

    RenderResourceReleaseService& service = GetRenderResourceReleaseService();
    service.RegisterQueue(&queue);

    auto proxy = std::make_shared<TestRenderProxy>(true);
    RenderResourceReleaseToken token = service.DeferRelease(proxy);

    ASSERT_TRUE(token.IsValid());
    EXPECT_FALSE(service.IsComplete(token));

    completedFence = 0;
    queue.ProcessCompleted();
    EXPECT_TRUE(service.IsComplete(token));

    service.UnregisterQueue(&queue);
}
