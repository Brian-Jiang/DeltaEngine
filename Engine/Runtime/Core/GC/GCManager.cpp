#include "Runtime/Core/GC/GCManager.h"

#include "Runtime/Core/DObject.h"
#include "Runtime/Core/GC/DObjectRegistry.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/ObjectReferenceWalk.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

#include <chrono>
#include <vector>

using namespace DeltaEngine;

void GCManager::Mark()
{
    m_state   = EGCState::Marking;
    m_marking = true;

    DObjectRegistry& registry = GetDObjectRegistry();

    for (DObject* obj : registry.GetAllLiveObjects())
        obj->SetGCMarkColor(EGCMarkColor::White);

    std::vector<DObject*> workQueue;
    for (const DObjectHandle& root : registry.GetRoots())
    {
        DObject* obj = registry.Resolve(root);
        if (obj && obj->GetGCMarkColor() == EGCMarkColor::White)
        {
            obj->SetGCMarkColor(EGCMarkColor::Gray);
            workQueue.push_back(obj);
        }
    }

    while (!workQueue.empty())
    {
        DObject* obj = workQueue.back();
        workQueue.pop_back();

        if (obj->GetGCMarkColor() == EGCMarkColor::Black)
            continue;

        obj->SetGCMarkColor(EGCMarkColor::Black);

        VisitResolvedObjectReferencesOnObject(obj,
            [&workQueue](DObject* target)
            {
                if (target && target->GetGCMarkColor() == EGCMarkColor::White)
                {
                    target->SetGCMarkColor(EGCMarkColor::Gray);
                    workQueue.push_back(target);
                }
            });
    }

    m_marking = false;
    m_state   = EGCState::Idle;
}

void GCManager::RequestCollect()
{
    if (m_state == EGCState::Idle && m_pendingDestroy.empty())
        m_collectRequested = true;
}

void GCManager::Tick()
{
    switch (m_state)
    {
    case EGCState::Idle:
        if (m_collectRequested)
        {
            m_collectRequested = false;
            Mark();
            BeginSweep();
        }
        break;
    case EGCState::Sweeping:
        DrainSweep();
        break;
    default:
        break;
    }
}

void GCManager::CollectGarbage()
{
    if (m_state != EGCState::Idle || !m_pendingDestroy.empty())
        return;

    Mark();
    BeginSweep();
    DrainSweepWithTimeout(kDefaultSweepTimeoutSeconds);
}

void GCManager::CollectAllForShutdown()
{
    // Finish any in-flight sweep so the pending-destroy list starts empty.
    DrainSweepWithTimeout(kDefaultSweepTimeoutSeconds);

    // Treat every live object as unreachable, then sweep everything. Roots are
    // intentionally ignored: this is a final teardown, not a reachability collect.
    DObjectRegistry& registry = GetDObjectRegistry();
    for (DObject* obj : registry.GetAllLiveObjects())
        obj->SetGCMarkColor(EGCMarkColor::White);

    BeginSweep();
    DrainSweepWithTimeout(kDefaultSweepTimeoutSeconds);
}

void GCManager::DrainPendingDestroyWithTimeout(double timeoutSeconds)
{
    if (!HasPendingDestroy() && m_state == EGCState::Idle)
        return;

    DrainSweepWithTimeout(timeoutSeconds);
}

void GCManager::DrainSweepWithTimeout(double timeoutSeconds)
{
    const auto deadline = std::chrono::steady_clock::now()
        + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<double>(timeoutSeconds));

    while (m_state == EGCState::Sweeping || HasPendingDestroy())
    {
        DrainSweep();

        if (!HasPendingDestroy() && m_state == EGCState::Idle)
            return;

        if (std::chrono::steady_clock::now() >= deadline)
        {
            DLOG(LogCore, ELogLevel::Error,
                "GC sweep did not complete within {:.1f}s; {} object(s) still pending. Forcing cleanup.",
                timeoutSeconds, m_pendingDestroy.size());
            ForceFinishPendingDestroy();
            return;
        }
    }
}

void GCManager::ForceFinishPendingDestroy()
{
    std::vector<DObject*> pending = std::move(m_pendingDestroy);
    m_pendingDestroy.clear();

    for (DObject* obj : pending)
    {
        if (!obj->IsReadyForFinishDestroy())
        {
            DClass* cls = obj->GetClass();
            const char* name = cls ? cls->GetName().c_str() : "DObject";
            DLOG(LogCore, ELogLevel::Warning,
                "GC force-finishing {} before IsReadyForFinishDestroy", name);
        }

        obj->FinishDestroy();
        GetReflectionRegistry().DestroyObject(obj);
    }

    m_state = EGCState::Idle;
}

void GCManager::BeginSweep()
{
    m_state = EGCState::Sweeping;

    DObjectRegistry& registry = GetDObjectRegistry();
    for (DObject* obj : registry.GetAllLiveObjects())
    {
        if (obj->GetGCMarkColor() != EGCMarkColor::White)
            continue;

        registry.RequestPendingKill(obj->GetGCHandle());
        obj->BeginDestroy();
        m_pendingDestroy.push_back(obj);
    }

    if (m_pendingDestroy.empty())
        m_state = EGCState::Idle;
}

size_t GCManager::DrainSweep()
{
    size_t finished = 0;

    auto it = m_pendingDestroy.begin();
    while (it != m_pendingDestroy.end())
    {
        DObject* obj = *it;
        if (obj->IsReadyForFinishDestroy())
        {
            obj->FinishDestroy();
            GetReflectionRegistry().DestroyObject(obj);
            it = m_pendingDestroy.erase(it);
            ++finished;
        }
        else
        {
            ++it;
        }
    }

    if (m_pendingDestroy.empty())
        m_state = EGCState::Idle;

    return finished;
}

GCManager& DeltaEngine::GetGCManager()
{
    static GCManager manager;
    return manager;
}
