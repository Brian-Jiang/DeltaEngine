#include "Runtime/Graphics/RenderResourceReleaseService.h"

#include "Runtime/Graphics/RenderProxy/RenderProxy.h"

using namespace DeltaEngine;

RenderResourceReleaseService& RenderResourceReleaseService::Get()
{
    static RenderResourceReleaseService service;
    return service;
}

void RenderResourceReleaseService::RegisterQueue(RenderResourceReleaseQueue* queue)
{
    m_queue = queue;
}

void RenderResourceReleaseService::UnregisterQueue(RenderResourceReleaseQueue* queue)
{
    if (m_queue == queue)
        m_queue = nullptr;
}

RenderResourceReleaseToken RenderResourceReleaseService::DeferRelease(std::shared_ptr<RenderProxy> proxy)
{
    if (!m_queue || !proxy)
        return RenderResourceReleaseToken::Invalid();

    return m_queue->Enqueue(std::move(proxy));
}

bool RenderResourceReleaseService::IsComplete(RenderResourceReleaseToken token) const
{
    if (!token.IsValid())
        return true;

    if (!m_queue)
        return true;

    return m_queue->IsComplete(token);
}
