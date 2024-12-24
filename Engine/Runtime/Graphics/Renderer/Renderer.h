#pragma once

#include "EngineIncludes.h"

#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Graphics/DXGraphicsContext.h"

DELTA_ENGINE_NS_BEGIN

class DWorld;

class Renderer : public SceneComponent
{
    friend class DWorld;

protected:
    virtual void InitGraphicState(DXGraphicsContext context) = 0;
    virtual void GatherDrawCalls(DXGraphicsContext context) = 0;
};

DELTA_ENGINE_NS_END