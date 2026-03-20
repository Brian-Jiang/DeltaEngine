#pragma once

#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/DVectorProperty.h"

DELTA_ENGINE_NS_BEGIN

template <typename Visitor>
void VisitUnresolvedObjectReferencesInProperty(
    DProperty* prop,
    void* valueAddress,
    Visitor&& visitor)
{
    if (!prop || !valueAddress)
        return;

    if (auto* ptrProp = dynamic_cast<DObjectPtrPropertyBase*>(prop))
    {
        ScriptPointer sp = ptrProp->GetUnresolvedPointer(valueAddress);
        if (!sp.IsNull())
            visitor(ptrProp, valueAddress, sp);
        return;
    }

    auto* vectorProp = dynamic_cast<DVectorPropertyBase*>(prop);
    if (!vectorProp)
        return;

    DProperty* innerProp = const_cast<DProperty*>(vectorProp->GetInnerProperty());
    if (!innerProp)
        return;

    const size_t elementCount = vectorProp->GetSize(valueAddress);
    for (size_t index = 0; index < elementCount; ++index)
    {
        void* elementAddress = vectorProp->GetElementAddress(valueAddress, index);
        if (!elementAddress)
            continue;

        VisitUnresolvedObjectReferencesInProperty(innerProp, elementAddress, visitor);
    }
}

template <typename Visitor>
void VisitUnresolvedObjectReferencesInStruct(
    DStruct* ds,
    DObject* obj,
    Visitor&& visitor)
{
    if (!ds || !obj)
        return;

    if (DStruct* parent = ds->GetSuper())
        VisitUnresolvedObjectReferencesInStruct(parent, obj, visitor);

    for (DProperty* prop = ds->GetOwnProperties(); prop; prop = prop->GetNext())
        VisitUnresolvedObjectReferencesInProperty(prop, obj, visitor);
}

DELTA_ENGINE_NS_END
