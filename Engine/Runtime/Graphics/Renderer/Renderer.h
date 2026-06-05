#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <optional>

#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Graphics/RenderResourceReleaseQueue.h"
#include "Runtime/Graphics/Shadow/ShadowView.h"

#include "Renderer.generated.h"

DELTA_ENGINE_NS_BEGIN

class DWorld;
class RenderProxy;
struct DXGraphicsContext;

DCLASS()
class DELTAENGINE_API Renderer : public SceneComponent
{
    DGENERATED_BODY(Renderer)
    friend class DWorld;

public:
    Renderer();

    DELTAENGINE_API void BeginDestroy() override;
    DELTAENGINE_API bool IsReadyForFinishDestroy() override;

    /// Rebuilds the render proxy used by the renderer for draw submission.
    virtual void CreateRenderProxy() = 0;

protected:
    /** One-time GPU-resource setup (PSOs, uploads). Invoked by the world before the first draw. */
    virtual void InitGraphicState(std::shared_ptr<DXGraphicsContext> context) = 0;
    /** Records per-frame draw calls into the active command list. */
    virtual void GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) = 0;
    /** Records depth-only draw calls for the supplied shadow view. Default no-op. */
    virtual void GatherShadowDrawCalls(std::shared_ptr<DXGraphicsContext> context, const ShadowView& view) {}

    /** Moves the owned render proxy out for deferred GPU release during GC sweep. */
    virtual std::shared_ptr<RenderProxy> DetachRenderProxyForRelease() = 0;

private:
    std::optional<RenderResourceReleaseToken> m_renderReleaseToken;
};

DELTA_ENGINE_NS_END
