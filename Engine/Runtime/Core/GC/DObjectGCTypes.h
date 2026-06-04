#pragma once

#include "EngineIncludes.h"

#include <cstdint>

DELTA_ENGINE_NS_BEGIN

using GCSlotIndex   = uint32_t;
using GCSlotVersion = uint32_t;

constexpr GCSlotIndex kInvalidGCSlot = UINT32_MAX;

enum class EGCSlotState : uint8_t
{
    Free,
    Live,
};

enum class EGCMarkColor : uint8_t
{
    White,
    Gray,
    Black,
};

struct DObjectHandle
{
    GCSlotIndex   m_slotIndex = kInvalidGCSlot;
    GCSlotVersion m_version   = 0;

    bool IsSet() const { return m_slotIndex != kInvalidGCSlot; }

    bool operator==(const DObjectHandle&) const = default;
};

DELTA_ENGINE_NS_END
