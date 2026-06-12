#pragma once

#include "EngineIncludes.h"

DELTA_ENGINE_NS_BEGIN

enum class DeferredLightingRootParameterType : uint32_t
{
    GBufferTextures,
    CameraCB,
    LightCB,
    PointLights,
    SpotLights,
    DirectionalLights,
    IBLTextures,
    ShadowMaps,
    ShadowCB,
    NumRootParameterTypes
};

DELTA_ENGINE_NS_END
