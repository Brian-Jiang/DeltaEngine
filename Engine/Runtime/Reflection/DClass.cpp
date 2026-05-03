#include "Runtime/Reflection/DClass.h"

#include "Runtime/Reflection/DFunction.h"

using namespace DeltaEngine;

DClass::DClass(std::string name,
               std::string superName,
               size_t classSize,
               size_t minAlignment,
               void (*constructFn)(void* address),
               void (*destructFn)(void* address),
               void (*copyFn)(void* dest, const void* src),
               DObject* classDefaultObject,
               bool isAbstract)
    : DStruct(std::move(name), std::move(superName), classSize, minAlignment)
    , m_constructFn(constructFn)
    , m_destructFn(destructFn)
    , m_copyFn(copyFn)
    , m_classDefaultObject(classDefaultObject)
    , m_abstract(isAbstract)
{
}

void DClass::AddFunction(DFunction* function)
{
    DELTA_VERIFY(function != nullptr);
    DELTA_VERIFY(!function->GetName().empty());

    auto [it, inserted] = m_functions.try_emplace(function->GetName(), function);
    if (!inserted)
    {
        DLOG(LogReflection, ELogLevel::Warning,
             "Ignoring duplicate reflected function '{}' on '{}'; Reflection names must be unique (keeping first overload)",
             function->GetName(), GetName());
        return;
    }

    function->m_declaringClass = this;
}

DFunction* DClass::FindFunctionByName(const std::string& name) const
{
    auto it = m_functions.find(name);
    if (it != m_functions.end())
        return it->second;

    DStruct* super = GetSuper();
    if (super)
    {
        DClass* superClass = dynamic_cast<DClass*>(super);
        if (superClass)
            return superClass->FindFunctionByName(name);
    }

    return nullptr;
}

bool DClass::IsChildOf(const DClass* other) const
{
    for (const DStruct* s = this; s; s = s->GetSuper())
    {
        if (s == other)
            return true;
    }
    return false;
}

bool DClass::IsAbstract() const { return m_abstract; }

const std::unordered_map<std::string, DFunction*>& DClass::GetFunctions() const { return m_functions; }

void DClass::ConstructObject(void* address) const
{
    DELTA_VERIFY(address != nullptr);
    DELTA_VERIFY(m_constructFn != nullptr);
    m_constructFn(address);
}

void DClass::DestroyObject(void* address) const
{
    DELTA_VERIFY(address != nullptr);
    if (m_destructFn)
        m_destructFn(address);
}

void DClass::CopyObject(void* dest, const void* src) const
{
    DELTA_VERIFY(dest != nullptr);
    DELTA_VERIFY(src != nullptr);
    DELTA_VERIFY(m_copyFn != nullptr);
    m_copyFn(dest, src);
}
