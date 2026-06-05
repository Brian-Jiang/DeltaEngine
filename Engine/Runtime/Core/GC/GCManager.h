#pragma once

#include "EngineIncludes.h"

#include <cstddef>
#include <cstdint>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class DObject;

enum class EGCState : uint8_t
{
    Idle,
    Marking,
    Sweeping,
};

/// Blocking, two-phase garbage collector. Phase 2 implements the reachability
/// mark; Phase 3 adds the sweep state machine (Idle -> Marking -> Sweeping) that
/// drains unreachable objects through a persistent pending-destroy list.
class GCManager
{
public:
    /// Marks every object reachable from the root set Black; everything else stays White.
    DELTAENGINE_API void Mark();

    /// Requests a collection. Only honored from Idle when no destroys are pending.
    DELTAENGINE_API void RequestCollect();

    /// Advances the state machine one frame: marks (single frame) then drains the
    /// sweep across subsequent frames.
    DELTAENGINE_API void Tick();

    /// Runs a full Mark -> Sweep -> drain cycle synchronously (used by tests).
    DELTAENGINE_API void CollectGarbage();

    EGCState GetState() const { return m_state; }

    /// True while a mark traversal is in progress (guards mid-mark allocations).
    bool IsMarking() const { return m_marking; }

    /// True while objects await FinishDestroy on the pending-destroy list.
    bool HasPendingDestroy() const { return !m_pendingDestroy.empty(); }

    /// Number of objects currently on the pending-destroy list.
    size_t GetPendingDestroyCount() const { return m_pendingDestroy.size(); }

private:
    /// Moves every unreachable (White) object to PendingKill, calls BeginDestroy,
    /// and parks it on the pending-destroy list.
    void BeginSweep();

    /// Calls FinishDestroy + frees every pending object that is ready; returns the
    /// count finished this call. Returns to Idle once the list drains.
    size_t DrainSweep();

    EGCState              m_state            = EGCState::Idle;
    bool                  m_marking          = false;
    bool                  m_collectRequested = false;
    std::vector<DObject*> m_pendingDestroy;
};

/// Returns the global GCManager instance.
DELTAENGINE_API GCManager& GetGCManager();

DELTA_ENGINE_NS_END
