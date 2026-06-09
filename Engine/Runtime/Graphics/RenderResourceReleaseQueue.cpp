#include "Runtime/Graphics/RenderResourceReleaseQueue.h"

#include "Runtime/Graphics/RenderProxy/RenderProxy.h"

using namespace DeltaEngine;

void RenderResourceReleaseQueue::SetFenceCompleteChecker(FenceCompleteFn checker)
{
    m_fenceCompleteChecker = std::move(checker);
}

void RenderResourceReleaseQueue::SetLastSubmittedFence(uint64_t fenceValue)
{
    m_lastSubmittedFence = fenceValue;
}

RenderResourceReleaseToken RenderResourceReleaseQueue::Enqueue(std::shared_ptr<RenderProxy> proxy)
{
    return Enqueue(std::move(proxy), m_lastSubmittedFence);
}

RenderResourceReleaseToken RenderResourceReleaseQueue::Enqueue(std::shared_ptr<RenderProxy> proxy, uint64_t fenceValue)
{
    if (!proxy)
        return RenderResourceReleaseToken::Invalid();

    RenderResourceReleaseToken token { m_nextTokenId++ };
    m_pending.push_back(Entry { token, fenceValue, std::move(proxy) });
    return token;
}

bool RenderResourceReleaseQueue::IsComplete(RenderResourceReleaseToken token) const
{
    if (!token.IsValid())
        return true;

    for (const Entry& entry : m_pending)
    {
        if (entry.token == token)
            return false;
    }

    return true;
}

size_t RenderResourceReleaseQueue::ProcessCompleted()
{
    if (!m_fenceCompleteChecker)
        return 0;

    size_t released = 0;

    auto it = m_pending.begin();
    while (it != m_pending.end())
    {
        if (m_fenceCompleteChecker(it->fenceValue))
        {
            it = m_pending.erase(it);
            ++released;
        }
        else
        {
            ++it;
        }
    }

    return released;
}

size_t RenderResourceReleaseQueue::GetPendingCount() const
{
    return m_pending.size();
}
