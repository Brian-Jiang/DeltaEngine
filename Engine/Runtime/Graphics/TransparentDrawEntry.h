#pragma once

#include "EngineIncludes.h"

#include <cstddef>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class MeshRenderProxy;

struct TransparentDrawEntry
{
    MeshRenderProxy* proxy = nullptr;
    size_t submeshIndex = 0;
    float sortDepth = 0.f;
};

DELTAENGINE_API void SortTransparentDrawEntriesDescending(std::vector<TransparentDrawEntry>& entries);

DELTA_ENGINE_NS_END
