#pragma once

#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/DStruct.h"
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
    void* basePtr,
    Visitor&& visitor)
{
    if (!ds || !basePtr)
        return;
    if (DStruct* parent = ds->GetSuper())
        VisitUnresolvedObjectReferencesInStruct(parent, basePtr, visitor);

    for (DProperty* prop = ds->GetOwnProperties(); prop; prop = prop->GetNext())
    {
        if (auto* dsp = dynamic_cast<DStructProperty*>(prop))
        {
            void* nested = static_cast<uint8_t*>(basePtr) + dsp->GetOffset();
            if (DStruct* inner = dsp->GetSchema())
                VisitUnresolvedObjectReferencesInStruct(inner, nested, visitor);
            continue;
        }
        void* slot = static_cast<uint8_t*>(basePtr) + prop->GetOffset();
        VisitUnresolvedObjectReferencesInProperty(prop, slot, visitor);
    }
}

template <typename Visitor>
void VisitUnresolvedDelegateBindingsInProperty(
    DProperty* prop,
    void* valueAddress,
    Visitor&& visitor)
{
    if (!prop || !valueAddress)
        return;
    if (auto* delegateProp = dynamic_cast<DDelegatePropertyBase*>(prop))
    {
        if (delegateProp->GetUnresolvedBindings(valueAddress))
            visitor(delegateProp, valueAddress);
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

        VisitUnresolvedDelegateBindingsInProperty(innerProp, elementAddress, visitor);
    }
}

template <typename Visitor>
void VisitUnresolvedDelegateBindingsInStruct(
    DStruct* ds,
    void* basePtr,
    Visitor&& visitor)
{
    if (!ds || !basePtr)
        return;
    if (DStruct* parent = ds->GetSuper())
        VisitUnresolvedDelegateBindingsInStruct(parent, basePtr, visitor);

    for (DProperty* prop = ds->GetOwnProperties(); prop; prop = prop->GetNext())
    {
        if (auto* dsp = dynamic_cast<DStructProperty*>(prop))
        {
            void* nested = static_cast<uint8_t*>(basePtr) + dsp->GetOffset();
            if (DStruct* inner = dsp->GetSchema())
                VisitUnresolvedDelegateBindingsInStruct(inner, nested, visitor);
            continue;
        }
        void* slot = static_cast<uint8_t*>(basePtr) + prop->GetOffset();
        VisitUnresolvedDelegateBindingsInProperty(prop, slot, visitor);
    }
}

template <typename Resolver>
void ResolveUnresolvedDelegateBindingsInStruct(
    DStruct* ds,
    void* basePtr,
    Resolver&& resolveObject)
{
    VisitUnresolvedDelegateBindingsInStruct(ds, basePtr,
        [&](DDelegatePropertyBase* prop, void* fieldAddr)
        {
            prop->ResolveBindings(fieldAddr, resolveObject);
        });
}

DELTA_ENGINE_NS_END
