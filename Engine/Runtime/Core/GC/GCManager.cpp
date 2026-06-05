#include "Runtime/Core/GC/GCManager.h"

#include "Runtime/Core/DObject.h"
#include "Runtime/Core/GC/DObjectRegistry.h"
#include "Runtime/Reflection/ObjectReferenceWalk.h"

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

GCManager& DeltaEngine::GetGCManager()
{
    static GCManager manager;
    return manager;
}
