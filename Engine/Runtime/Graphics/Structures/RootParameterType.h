#pragma once

#include "EngineIncludes.h"

DELTA_ENGINE_NS_BEGIN

enum class RootParameterType
{
    /// Camera constant buffer slot.
    CameraCB,

    /// Bound texture slot.
    Texture,

    /// Per-object constant buffer slot.
    ObjectCB,

    /// Light count constant buffer slot.
    LightCB,

    /// Point light buffer slot.
    PointLights,

    /// Spot light buffer slot.
    SpotLights,

    /// Directional light buffer slot.
    DirectionalLights,

    /// Per-material constant buffer slot.
    MaterialCB,

    /// Total number of root parameters.
    NumRootParameterTypes
};

DELTA_ENGINE_NS_END
