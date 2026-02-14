#pragma once

#include "EngineIncludes.h"

DELTA_ENGINE_NS_BEGIN

enum class RootParameterType
{
    CameraCB,
    Texture,
    ObjectCB,
    LightCB,
    PointLights,
    SpotLights,
    DirectionalLights,

    NumRootParameterTypes
};

DELTA_ENGINE_NS_END
