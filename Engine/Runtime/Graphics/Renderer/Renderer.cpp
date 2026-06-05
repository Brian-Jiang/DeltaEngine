#include "Renderer.h"

#include "Runtime/Graphics/RenderProxy/RenderProxy.h"
#include "Runtime/Graphics/RenderResourceReleaseService.h"

using namespace DeltaEngine;

Renderer::Renderer()
    : SceneComponent()
{
}

void Renderer::BeginDestroy()
{
    if (m_renderReleaseToken.has_value())
        return;

    std::shared_ptr<RenderProxy> proxy = DetachRenderProxyForRelease();
    if (!proxy)
        return;

    proxy->ReleaseSharedReferences();

    if (proxy->HasExclusiveGPUResources())
        m_renderReleaseToken = GetRenderResourceReleaseService().DeferRelease(std::move(proxy));
}

bool Renderer::IsReadyForFinishDestroy()
{
    if (!m_renderReleaseToken.has_value())
        return true;

    return GetRenderResourceReleaseService().IsComplete(*m_renderReleaseToken);
}
