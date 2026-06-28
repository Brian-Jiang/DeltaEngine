#pragma once

#include "Runtime/EngineIncludes.h"

#include <cstdint>

DELTA_ENGINE_NS_BEGIN

struct ShadowAtlasSettings
{
    uint32_t atlasSize = 4096;
    uint32_t directionalTileSize = 2048;
    uint32_t spotTileSize = 1024;
    uint32_t pointFaceSize = 512;
    uint32_t pointCubeCount = 8;
};

DELTA_ENGINE_NS_END
