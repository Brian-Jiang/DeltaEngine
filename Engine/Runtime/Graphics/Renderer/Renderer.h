#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Graphics/DXGraphicsContext.h"

DELTA_ENGINE_NS_BEGIN

class DWorld;

class Renderer : public SceneComponent
{
    friend class DWorld;

protected:
    virtual void InitGraphicState(std::shared_ptr<DXGraphicsContext> context) = 0;
    virtual void GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) = 0;
};

DELTA_ENGINE_NS_END
