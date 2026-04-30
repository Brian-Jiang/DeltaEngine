#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/Shadow/ShadowView.h"

#include <memory>
#include <vector>

DELTA_ENGINE_NS_BEGIN

struct DXGraphicsContext;

/** Base class for GPU-facing render objects that record per-frame draw work for a scene entity. */
class RenderProxy
{
public:
    virtual ~RenderProxy() = default;

    /** One-time GPU resource setup (PSOs, uploads). Default no-op. */
    virtual void Initialize(std::shared_ptr<DXGraphicsContext> renderContext) {}

    /** Per-frame work that prepares the context before any draw is recorded (camera CB, light buffers). Default no-op. */
    virtual void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext) {}

    /** Per-frame work that records draw calls into the active command list. Default no-op. */
    virtual void GatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext) {}

    virtual void GatherShadowViews(std::shared_ptr<DXGraphicsContext> ctx, std::vector<ShadowView>& outViews) {}

    virtual void WriteShadowParams(std::shared_ptr<DXGraphicsContext> ctx, const ShadowAllocation& alloc) {}

    virtual void GatherShadowDrawCalls(std::shared_ptr<DXGraphicsContext> ctx, const ShadowView& view) {}
};

DELTA_ENGINE_NS_END
