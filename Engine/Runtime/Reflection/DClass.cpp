#include "Runtime/Reflection/DClass.h"

#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/DFunction.h"

using namespace DeltaEngine;

DClass::DClass(std::string name,
               DClass* super,
               size_t classSize,
               size_t minAlignment,
               void (*constructFn)(void* address),
               void (*destructFn)(void* address),
               void (*copyFn)(void* dest, const void* src),
               DObject* classDefaultObject)
    : m_name(std::move(name))
    , m_super(super)
    , m_properties(nullptr)
    , m_ownProperties(nullptr)
    , m_classSize(classSize)
    , m_minAlignment(minAlignment)
    , m_constructFn(constructFn)
    , m_destructFn(destructFn)
    , m_copyFn(copyFn)
    , m_classDefaultObject(classDefaultObject)
{
}

void DClass::AddProperty(DProperty* property)
{
    property->m_declaringClass = this;

    property->m_next = m_ownProperties;
    m_ownProperties = property;
}

DProperty* DClass::FindPropertyByName(const std::string& name) const
{
    for (DProperty* prop = m_ownProperties; prop; prop = prop->m_next)
    {
        if (prop->m_name == name)
            return prop;
    }

    if (m_super)
        return m_super->FindPropertyByName(name);

    return nullptr;
}

void DClass::AddFunction(DFunction* function)
{
    function->m_declaringClass = this;
    m_functions[function->GetName()] = function;
}

DFunction* DClass::FindFunctionByName(const std::string& name) const
{
    auto it = m_functions.find(name);
    if (it != m_functions.end())
        return it->second;

    if (m_super)
        return m_super->FindFunctionByName(name);

    return nullptr;
}

bool DClass::IsChildOf(const DClass* other) const
{
    for (const DClass* cls = this; cls; cls = cls->m_super)
    {
        if (cls == other)
            return true;
    }
    return false;
}

const std::string& DClass::GetName() const { return m_name; }
DClass* DClass::GetSuper() const { return m_super; }
size_t DClass::GetClassSize() const { return m_classSize; }
size_t DClass::GetMinAlignment() const { return m_minAlignment; }
DProperty* DClass::GetProperties() const { return m_properties; }
DProperty* DClass::GetOwnProperties() const { return m_ownProperties; }

void DClass::ConstructObject(void* address) const { m_constructFn(address); }
void DClass::DestroyObject(void* address) const { m_destructFn(address); }
void DClass::CopyObject(void* dest, const void* src) const { m_copyFn(dest, src); }
