#pragma once

#include "Runtime/EngineIncludes.h"

DELTA_ENGINE_NS_BEGIN

enum class GBufferRootParameterType : uint32_t
{
    CameraCB,
    ObjectCB,
    MaterialCB,
    Texture,
    NumRootParameterTypes
};

DELTA_ENGINE_NS_END
