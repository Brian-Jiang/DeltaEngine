#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/RenderResourceReleaseQueue.h"

#include <memory>

DELTA_ENGINE_NS_BEGIN

class RenderProxy;

/// Global accessor so DObjects can defer render-proxy release without holding DXRenderManager.
class RenderResourceReleaseService
{
public:
    DELTAENGINE_API static RenderResourceReleaseService& Get();

    DELTAENGINE_API void RegisterQueue(RenderResourceReleaseQueue* queue);
    DELTAENGINE_API void UnregisterQueue(RenderResourceReleaseQueue* queue);

    DELTAENGINE_API RenderResourceReleaseToken DeferRelease(std::shared_ptr<RenderProxy> proxy);
    DELTAENGINE_API bool IsComplete(RenderResourceReleaseToken token) const;

private:
    RenderResourceReleaseQueue* m_queue = nullptr;
};

DELTA_ENGINE_NS_END
