#pragma once

#include "EngineIncludes.h"

#include "Graphics/Shadow/ShadowView.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

DELTA_ENGINE_NS_BEGIN

struct ShadowMapTileRegion
{
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

class ShadowMapAllocator
{
public:
    DELTAENGINE_API void Reset(uint32_t atlasWidth, uint32_t atlasHeight, uint32_t tileSize);
    DELTAENGINE_API int32_t Allocate(uint32_t requestedEdgePx, ShadowMapTileRegion& outRegion);
    DELTAENGINE_API void Free(int32_t slotId);

    DELTAENGINE_API static void FillAtlasUVRect(uint32_t atlasWidth, uint32_t atlasHeight,
        const ShadowMapTileRegion& region, ShadowAllocation& outAlloc);

    DELTAENGINE_API uint32_t TileSize() const { return m_tileSize; }
    DELTAENGINE_API uint32_t AtlasWidth() const { return m_atlasWidth; }
    DELTAENGINE_API uint32_t AtlasHeight() const { return m_atlasHeight; }

private:
    bool Fits(uint32_t cellX, uint32_t cellY, uint32_t span) const;
    void Occupy(uint32_t cellX, uint32_t cellY, uint32_t span, bool value);

    uint32_t m_atlasWidth = 0;
    uint32_t m_atlasHeight = 0;
    uint32_t m_tileSize = 0;
    uint32_t m_tilesX = 0;
    uint32_t m_tilesY = 0;
    std::vector<uint8_t> m_occupied;
    uint32_t m_nextSlotId = 0;

    struct RegionCells
    {
        uint32_t cellX = 0;
        uint32_t cellY = 0;
        uint32_t span = 0;
    };
    std::unordered_map<int32_t, RegionCells> m_slots;
};

class PointSliceAllocator
{
public:
    DELTAENGINE_API void Reset(uint32_t maxCubes);
    DELTAENGINE_API int32_t Allocate();
    DELTAENGINE_API void Free(int32_t cubeIndex);

    DELTAENGINE_API uint32_t MaxCubes() const { return m_maxCubes; }

private:
    uint32_t m_maxCubes = 0;
    std::vector<uint8_t> m_inUse;
};

DELTA_ENGINE_NS_END
