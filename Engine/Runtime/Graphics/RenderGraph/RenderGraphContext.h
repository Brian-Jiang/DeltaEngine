#pragma once

#include "EngineIncludes.h"

DELTA_ENGINE_NS_BEGIN

class CommandList;
struct DXGraphicsContext;

struct RenderGraphContext
{
    CommandList* commandList = nullptr;
    DXGraphicsContext* graphicsContext = nullptr;
};

DELTA_ENGINE_NS_END
