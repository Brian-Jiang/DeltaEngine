#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/RenderProxy/RenderProxy.h"
#include "Runtime/Graphics/Renderer/Renderer.h"

#include "GCRenderLifecycleTestRenderer.generated.h"

DELTA_ENGINE_NS_BEGIN

/// Test renderer with an injectable render proxy for GC deferred-release sweep tests.
DCLASS()
class GCRenderLifecycleTestRenderer : public Renderer
{
    DGENERATED_BODY(GCRenderLifecycleTestRenderer)

public:
    DELTAENGINE_API void CreateRenderProxy() override;

    DELTAENGINE_API void BeginDestroy() override;
    DELTAENGINE_API static void Reset();

    DELTAENGINE_API static int  s_beginDestroyCount;
    DELTAENGINE_API static int  s_finishDestroyCount;
    DELTAENGINE_API static bool s_proxyHasExclusiveResources;

protected:
    void InitGraphicState(std::shared_ptr<DXGraphicsContext> context) override {}
    void GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) override {}
    std::shared_ptr<RenderProxy> DetachRenderProxyForRelease() override;

    DELTAENGINE_API void FinishDestroy() override;

private:
    std::shared_ptr<RenderProxy> m_testProxy;
};

DELTA_ENGINE_NS_END
