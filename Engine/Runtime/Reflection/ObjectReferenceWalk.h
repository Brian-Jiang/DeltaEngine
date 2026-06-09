#pragma once

#include "Runtime/Core/DObject.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/DStruct.h"
#include "Runtime/Reflection/DVectorProperty.h"

DELTA_ENGINE_NS_BEGIN

template <typename Visitor>
void VisitResolvedObjectReferencesInStruct(DStruct* ds, void* basePtr, Visitor&& visitor);

template <typename Visitor>
void VisitResolvedObjectReferencesInProperty(
    DProperty* prop,
    void* containerPtr,
    Visitor&& visitor)
{
    if (!prop || !containerPtr)
        return;

    if (dynamic_cast<DObjectPtrPropertyBase*>(prop))
    {
        if (DObject* target = prop->GetObjectPointer(containerPtr))
            visitor(target);
        return;
    }

    if (auto* dsp = dynamic_cast<DStructProperty*>(prop))
    {
        if (DStruct* inner = dsp->GetSchema())
            VisitResolvedObjectReferencesInStruct(inner, containerPtr, visitor);
        return;
    }

    auto* vectorProp = dynamic_cast<DVectorPropertyBase*>(prop);
    if (!vectorProp)
        return;

    DProperty* innerProp = const_cast<DProperty*>(vectorProp->GetInnerProperty());
    if (!innerProp)
        return;

    const size_t elementCount = vectorProp->GetSize(containerPtr);
    for (size_t index = 0; index < elementCount; ++index)
    {
        void* elementAddress = vectorProp->GetElementAddress(containerPtr, index);
        if (!elementAddress)
            continue;

        VisitResolvedObjectReferencesInProperty(innerProp, elementAddress, visitor);
    }
}

template <typename Visitor>
void VisitResolvedObjectReferencesInStruct(
    DStruct* ds,
    void* basePtr,
    Visitor&& visitor)
{
    if (!ds || !basePtr)
        return;
    if (DStruct* parent = ds->GetSuper())
        VisitResolvedObjectReferencesInStruct(parent, basePtr, visitor);

    for (DProperty* prop = ds->GetOwnProperties(); prop; prop = prop->GetNext())
    {
        if (auto* dsp = dynamic_cast<DStructProperty*>(prop))
        {
            void* nested = static_cast<uint8_t*>(basePtr) + dsp->GetOffset();
            if (DStruct* inner = dsp->GetSchema())
                VisitResolvedObjectReferencesInStruct(inner, nested, visitor);
            continue;
        }

        if (dynamic_cast<DObjectPtrPropertyBase*>(prop))
        {
            VisitResolvedObjectReferencesInProperty(prop, basePtr, visitor);
            continue;
        }

        void* slot = static_cast<uint8_t*>(basePtr) + prop->GetOffset();
        VisitResolvedObjectReferencesInProperty(prop, slot, visitor);
    }
}

template <typename Visitor>
void VisitResolvedObjectReferencesOnObject(DObject* obj, Visitor&& visitor)
{
    if (!obj)
        return;
    if (DClass* cls = obj->GetClass())
        VisitResolvedObjectReferencesInStruct(cls, obj, visitor);
}

DELTA_ENGINE_NS_END
