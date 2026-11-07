#include "Runtime/Reflection/DClass.h"

#include "Runtime/Reflection/DFunction.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Logging/LogChannels.h"

using namespace DeltaEngine;

namespace
{
    bool ParamsMatch(const DFunction& fn, std::span<const std::string_view> paramTypes)
    {
        const auto& params = fn.GetParams();
        if (params.size() != paramTypes.size())
            return false;
        for (size_t i = 0; i < params.size(); ++i)
        {
            if (params[i] == nullptr)
                return false;
            if (params[i]->GetType() != paramTypes[i])
                return false;
        }
        return true;
    }

    bool SignaturesEqual(const DFunction& a, const DFunction& b)
    {
        const auto& pa = a.GetParams();
        const auto& pb = b.GetParams();
        if (pa.size() != pb.size())
            return false;
        for (size_t i = 0; i < pa.size(); ++i)
        {
            if (pa[i] == nullptr || pb[i] == nullptr)
                return false;
            if (pa[i]->GetType() != pb[i]->GetType())
                return false;
        }
        return true;
    }
}

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

    auto& bucket = m_functions[function->GetName()];
    for (DFunction* existing : bucket)
    {
        if (existing && SignaturesEqual(*existing, *function))
        {
            DLOG(LogReflection, ELogLevel::Warning,
                 "Ignoring duplicate reflected function '{}' on '{}' with identical signature",
                 function->GetName(), GetName());
            return;
        }
    }

    bucket.push_back(function);
    function->m_declaringClass = this;
}

DFunction* DClass::FindFunctionByName(const std::string& name) const
{
    auto it = m_functions.find(name);
    if (it != m_functions.end() && !it->second.empty())
        return it->second.front();

    DStruct* super = GetSuper();
    if (super)
    {
        DClass* superClass = dynamic_cast<DClass*>(super);
        if (superClass)
            return superClass->FindFunctionByName(name);
    }

    return nullptr;
}

std::vector<DFunction*> DClass::FindOverloads(const std::string& name) const
{
    auto it = m_functions.find(name);
    if (it == m_functions.end())
        return {};
    return it->second;
}

DFunction* DClass::FindFunction(const std::string& name,
                                std::span<const std::string_view> paramTypes) const
{
    auto it = m_functions.find(name);
    if (it != m_functions.end())
    {
        for (DFunction* fn : it->second)
        {
            if (fn && ParamsMatch(*fn, paramTypes))
                return fn;
        }
    }

    DStruct* super = GetSuper();
    if (super)
    {
        if (DClass* superClass = dynamic_cast<DClass*>(super))
            return superClass->FindFunction(name, paramTypes);
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

std::vector<DFunction*> DClass::GetFunctions() const
{
    std::vector<DFunction*> out;
    for (const auto& [name, bucket] : m_functions)
    {
        for (DFunction* fn : bucket)
            out.push_back(fn);
    }
    return out;
}

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
