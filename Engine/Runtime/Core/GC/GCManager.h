#pragma once

#include "EngineIncludes.h"

#include <cstdint>

DELTA_ENGINE_NS_BEGIN

enum class EGCState : uint8_t
{
    Idle,
    Marking,
    Sweeping,
};

/// Blocking, two-phase garbage collector. Phase 2 implements the reachability
/// mark: a color-flip followed by a non-recursive traversal of reflected
/// references seeded from the registry root set.
class GCManager
{
public:
    /// Marks every object reachable from the root set Black; everything else stays White.
    DELTAENGINE_API void Mark();

    EGCState GetState() const { return m_state; }

    /// True while a mark traversal is in progress (guards mid-mark allocations).
    bool IsMarking() const { return m_marking; }

private:
    EGCState m_state   = EGCState::Idle;
    bool     m_marking = false;
};

/// Returns the global GCManager instance.
DELTAENGINE_API GCManager& GetGCManager();

DELTA_ENGINE_NS_END
