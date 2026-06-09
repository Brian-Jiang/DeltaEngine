#pragma once

#include "EngineIncludes.h"

DELTA_ENGINE_NS_BEGIN

class CommandList;

struct RenderGraphContext
{
    CommandList* commandList = nullptr;
};

DELTA_ENGINE_NS_END
