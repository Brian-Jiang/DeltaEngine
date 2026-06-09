#include "Runtime/Test/GCRenderLifecycleTestRenderer.h"

using namespace DeltaEngine;

namespace
{
class GCTestRenderProxy : public RenderProxy
{
public:
    explicit GCTestRenderProxy(bool hasExclusiveResources)
        : m_hasExclusiveResources(hasExclusiveResources)
    {
    }

    bool HasExclusiveGPUResources() const override { return m_hasExclusiveResources; }

private:
    bool m_hasExclusiveResources = false;
};
}

int  GCRenderLifecycleTestRenderer::s_beginDestroyCount              = 0;
int  GCRenderLifecycleTestRenderer::s_finishDestroyCount             = 0;
bool GCRenderLifecycleTestRenderer::s_proxyHasExclusiveResources    = true;

void GCRenderLifecycleTestRenderer::CreateRenderProxy()
{
    m_testProxy = std::make_shared<GCTestRenderProxy>(s_proxyHasExclusiveResources);
}

void GCRenderLifecycleTestRenderer::BeginDestroy()
{
    ++s_beginDestroyCount;
    Renderer::BeginDestroy();
}

std::shared_ptr<RenderProxy> GCRenderLifecycleTestRenderer::DetachRenderProxyForRelease()
{
    return std::move(m_testProxy);
}

void GCRenderLifecycleTestRenderer::FinishDestroy()
{
    ++s_finishDestroyCount;
}

void GCRenderLifecycleTestRenderer::Reset()
{
    s_beginDestroyCount              = 0;
    s_finishDestroyCount             = 0;
    s_proxyHasExclusiveResources     = true;
}
