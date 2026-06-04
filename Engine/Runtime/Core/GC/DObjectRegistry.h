#pragma once

#include "EngineIncludes.h"

#include "Runtime/Core/GC/DObjectGCTypes.h"

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class DObject;

struct GCSlot
{
    DObject*      m_object    = nullptr;
    GCSlotVersion m_version   = 0;
    EGCSlotState  m_state     = EGCSlotState::Free;
    uint32_t      m_rootCount = 0;
};

/// Versioned slot registry tracking every live DObject for the garbage collector.
/// Slots are stored in fixed-size buckets so slot addresses stay stable as the
/// registry grows. Freeing a slot bumps its version, invalidating any handle that
/// still points at the old occupant (weak/strong pointer resolution returns null).
class DObjectRegistry
{
public:
    static constexpr GCSlotIndex kBucketSize = 1024;

    /// Allocates (or reuses) a slot for the object and writes the handle back into it.
    DELTAENGINE_API DObjectHandle RegisterObject(DObject* object);

    /// Releases the slot referenced by the handle, bumping its version.
    DELTAENGINE_API void FreeSlot(const DObjectHandle& handle);

    /// Returns the live object for the handle, or nullptr if it was freed/reused.
    DELTAENGINE_API DObject* Resolve(const DObjectHandle& handle) const;

    /// Returns true when the handle still refers to a live slot of the same version.
    DELTAENGINE_API bool IsValid(const DObjectHandle& handle) const;

    /// Adds a root reference to the slot (idempotent reference-counted root set).
    DELTAENGINE_API void AddRoot(const DObjectHandle& handle);

    /// Removes a root reference from the slot.
    DELTAENGINE_API void RemoveRoot(const DObjectHandle& handle);

    /// Returns the handles of every slot with a non-zero root count.
    DELTAENGINE_API std::vector<DObjectHandle> GetRoots() const;

    /// Number of currently live (allocated) slots.
    DELTAENGINE_API size_t GetLiveCount() const;

private:
    GCSlot*       FindSlot(const DObjectHandle& handle);
    const GCSlot* FindSlot(const DObjectHandle& handle) const;
    GCSlot&       SlotAt(GCSlotIndex index);

    using Bucket = std::array<GCSlot, kBucketSize>;

    std::vector<std::unique_ptr<Bucket>> m_buckets;
    std::vector<GCSlotIndex>             m_freeList;
    GCSlotIndex                          m_capacity  = 0;
    size_t                               m_liveCount = 0;
};

/// Returns the global DObject registry instance.
DELTAENGINE_API DObjectRegistry& GetDObjectRegistry();

DELTA_ENGINE_NS_END
