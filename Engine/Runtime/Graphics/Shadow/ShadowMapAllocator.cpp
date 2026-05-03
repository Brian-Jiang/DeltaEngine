#include "Runtime/Graphics/Shadow/ShadowMapAllocator.h"

#include <algorithm>

using namespace DeltaEngine;

void ShadowMapAllocator::Reset(uint32_t atlasWidth, uint32_t atlasHeight, uint32_t tileSize)
{
    m_slots.clear();
    m_nextSlotId = 0;
    m_atlasWidth = std::max(1u, atlasWidth);
    m_atlasHeight = std::max(1u, atlasHeight);
    m_tileSize = std::max(1u, tileSize);
    m_tilesX = (m_atlasWidth + m_tileSize - 1u) / m_tileSize;
    m_tilesY = (m_atlasHeight + m_tileSize - 1u) / m_tileSize;
    m_occupied.assign(m_tilesX * m_tilesY, 0);
}

bool ShadowMapAllocator::Fits(uint32_t cellX, uint32_t cellY, uint32_t span) const
{
    if (cellX + span > m_tilesX || cellY + span > m_tilesY)
        return false;
    for (uint32_t dy = 0; dy < span; ++dy) {
        for (uint32_t dx = 0; dx < span; ++dx) {
            const uint32_t idx = (cellY + dy) * m_tilesX + (cellX + dx);
            if (m_occupied[idx])
                return false;
        }
    }
    return true;
}

void ShadowMapAllocator::Occupy(uint32_t cellX, uint32_t cellY, uint32_t span, bool value)
{
    const uint8_t v = value ? 1u : 0u;
    for (uint32_t dy = 0; dy < span; ++dy) {
        for (uint32_t dx = 0; dx < span; ++dx) {
            const uint32_t idx = (cellY + dy) * m_tilesX + (cellX + dx);
            m_occupied[idx] = v;
        }
    }
}

int32_t ShadowMapAllocator::Allocate(uint32_t requestedEdgePx, ShadowMapTileRegion& outRegion)
{
    if (m_tileSize == 0 || m_tilesX == 0 || m_tilesY == 0)
        return -1;

    const uint32_t needPx = std::max(1u, requestedEdgePx);
    const uint32_t span = (needPx + m_tileSize - 1u) / m_tileSize;

    for (uint32_t cy = 0; cy + span <= m_tilesY; ++cy) {
        for (uint32_t cx = 0; cx + span <= m_tilesX; ++cx) {
            if (!Fits(cx, cy, span))
                continue;

            Occupy(cx, cy, span, true);
            const int32_t id = static_cast<int32_t>(m_nextSlotId++);
            m_slots[id] = RegionCells { cx, cy, span };

            outRegion.x = cx * m_tileSize;
            outRegion.y = cy * m_tileSize;
            outRegion.width = span * m_tileSize;
            outRegion.height = span * m_tileSize;

            if (outRegion.x + outRegion.width > m_atlasWidth)
                outRegion.width = m_atlasWidth - outRegion.x;
            if (outRegion.y + outRegion.height > m_atlasHeight)
                outRegion.height = m_atlasHeight - outRegion.y;

            return id;
        }
    }
    return -1;
}

void ShadowMapAllocator::Free(int32_t slotId)
{
    auto it = m_slots.find(slotId);
    if (it == m_slots.end())
    {
        DLOG(LogShadow, ELogLevel::Warning,
            "ShadowMapAllocator::Free ignored unknown slotId={} (activeSlots={}, expected id returned by Allocate)",
            slotId, m_slots.size());
        return;
    }
    const RegionCells& r = it->second;
    Occupy(r.cellX, r.cellY, r.span, false);
    m_slots.erase(it);
}

void ShadowMapAllocator::FillAtlasUVRect(uint32_t atlasWidth, uint32_t atlasHeight,
    const ShadowMapTileRegion& region, ShadowAllocation& outAlloc)
{
    const float aw = static_cast<float>(std::max(1u, atlasWidth));
    const float ah = static_cast<float>(std::max(1u, atlasHeight));
    outAlloc.atlasUVRect.x = static_cast<float>(region.x) / aw;
    outAlloc.atlasUVRect.y = static_cast<float>(region.y) / ah;
    outAlloc.atlasUVRect.z = static_cast<float>(region.width) / aw;
    outAlloc.atlasUVRect.w = static_cast<float>(region.height) / ah;
}

void PointSliceAllocator::Reset(uint32_t maxCubes)
{
    m_maxCubes = maxCubes;
    m_inUse.assign(maxCubes, 0);
}

int32_t PointSliceAllocator::Allocate()
{
    for (uint32_t i = 0; i < m_maxCubes; ++i) {
        if (!m_inUse[i]) {
            m_inUse[i] = 1;
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

void PointSliceAllocator::Free(int32_t cubeIndex)
{
    if (cubeIndex < 0 || static_cast<uint32_t>(cubeIndex) >= m_maxCubes)
        return;
    m_inUse[static_cast<uint32_t>(cubeIndex)] = 0;
}
