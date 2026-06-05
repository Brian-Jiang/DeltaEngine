#include "Runtime/Core/GC/DObjectRegistry.h"

#include "Runtime/Core/DObject.h"

using namespace DeltaEngine;

GCSlot& DObjectRegistry::SlotAt(GCSlotIndex index)
{
    const GCSlotIndex bucket = index / kBucketSize;
    const GCSlotIndex offset = index % kBucketSize;
    return (*m_buckets[bucket])[offset];
}

GCSlot* DObjectRegistry::FindSlot(const DObjectHandle& handle)
{
    if (!handle.IsSet() || handle.m_slotIndex >= m_capacity)
        return nullptr;

    GCSlot& slot = SlotAt(handle.m_slotIndex);
    if (slot.m_version != handle.m_version)
        return nullptr;

    return &slot;
}

const GCSlot* DObjectRegistry::FindSlot(const DObjectHandle& handle) const
{
    return const_cast<DObjectRegistry*>(this)->FindSlot(handle);
}

DObjectHandle DObjectRegistry::RegisterObject(DObject* object)
{
    if (!object)
    {
        DLOG(LogCore, ELogLevel::Warning, "DObjectRegistry::RegisterObject ignored null object");
        return {};
    }

    GCSlotIndex index;
    if (!m_freeList.empty())
    {
        index = m_freeList.back();
        m_freeList.pop_back();
    }
    else
    {
        if (m_capacity % kBucketSize == 0)
            m_buckets.push_back(std::make_unique<Bucket>());

        index = m_capacity;
        ++m_capacity;
    }

    GCSlot& slot = SlotAt(index);
    slot.m_object    = object;
    slot.m_state     = EGCSlotState::Live;
    slot.m_rootCount = 0;

    DObjectHandle handle{ index, slot.m_version };
    object->SetGCHandle(handle);
    ++m_liveCount;
    return handle;
}

void DObjectRegistry::FreeSlot(const DObjectHandle& handle)
{
    GCSlot* slot = FindSlot(handle);
    if (!slot)
        return;

    if (slot->m_state == EGCSlotState::Live)
        --m_liveCount;
    else if (slot->m_state != EGCSlotState::PendingKill)
        return;

    slot->m_object    = nullptr;
    slot->m_state     = EGCSlotState::Free;
    slot->m_rootCount = 0;
    ++slot->m_version;

    m_freeList.push_back(handle.m_slotIndex);
}

void DObjectRegistry::RequestPendingKill(const DObjectHandle& handle)
{
    GCSlot* slot = FindSlot(handle);
    if (!slot || slot->m_state != EGCSlotState::Live)
        return;

    slot->m_state     = EGCSlotState::PendingKill;
    slot->m_rootCount = 0;
    --m_liveCount;
}

DObject* DObjectRegistry::Resolve(const DObjectHandle& handle) const
{
    const GCSlot* slot = FindSlot(handle);
    if (!slot || slot->m_state != EGCSlotState::Live)
        return nullptr;

    return slot->m_object;
}

bool DObjectRegistry::IsValid(const DObjectHandle& handle) const
{
    const GCSlot* slot = FindSlot(handle);
    return slot != nullptr && slot->m_state == EGCSlotState::Live;
}

void DObjectRegistry::AddRoot(const DObjectHandle& handle)
{
    GCSlot* slot = FindSlot(handle);
    if (!slot || slot->m_state != EGCSlotState::Live)
        return;

    ++slot->m_rootCount;
}

void DObjectRegistry::RemoveRoot(const DObjectHandle& handle)
{
    GCSlot* slot = FindSlot(handle);
    if (!slot || slot->m_state != EGCSlotState::Live)
        return;

    if (slot->m_rootCount > 0)
        --slot->m_rootCount;
}

std::vector<DObjectHandle> DObjectRegistry::GetRoots() const
{
    std::vector<DObjectHandle> roots;
    for (GCSlotIndex index = 0; index < m_capacity; ++index)
    {
        const GCSlot& slot = const_cast<DObjectRegistry*>(this)->SlotAt(index);
        if (slot.m_state == EGCSlotState::Live && slot.m_rootCount > 0)
            roots.push_back(DObjectHandle{ index, slot.m_version });
    }
    return roots;
}

std::vector<DObject*> DObjectRegistry::GetAllLiveObjects() const
{
    std::vector<DObject*> objects;
    objects.reserve(m_liveCount);
    for (GCSlotIndex index = 0; index < m_capacity; ++index)
    {
        const GCSlot& slot = const_cast<DObjectRegistry*>(this)->SlotAt(index);
        if (slot.m_state == EGCSlotState::Live && slot.m_object)
            objects.push_back(slot.m_object);
    }
    return objects;
}

size_t DObjectRegistry::GetLiveCount() const
{
    return m_liveCount;
}

DObjectRegistry& DeltaEngine::GetDObjectRegistry()
{
    static DObjectRegistry registry;
    return registry;
}
