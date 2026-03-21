#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "Runtime/Core/SceneComponent.h"

#include "Renderer.generated.h"

DELTA_ENGINE_NS_BEGIN

class DWorld;
struct DXGraphicsContext;

DCLASS()
class Renderer : public SceneComponent
{
    DGENERATED_BODY(Renderer)
    friend class DWorld;

public:
    Renderer();

    /// Rebuilds the render proxy used by the renderer for draw submission.
    virtual void CreateRenderProxy() = 0;

protected:
    virtual void InitGraphicState(std::shared_ptr<DXGraphicsContext> context) = 0;
    virtual void GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) = 0;
};

DELTA_ENGINE_NS_END
