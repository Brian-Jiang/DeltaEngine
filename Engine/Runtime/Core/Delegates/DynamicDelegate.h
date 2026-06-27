#pragma once

#include "EngineIncludes.h"

#include "Runtime/Core/DObject.h"
#include "Runtime/Core/GC/DObjectRegistry.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DFunction.h"

#include <cstdint>
#include <string>
#include <vector>

DELTA_ENGINE_NS_BEGIN

struct FDynamicDelegateBinding
{
    DObjectHandle m_objectHandle{};
    std::string   m_functionName;

    bool operator==(const FDynamicDelegateBinding&) const = default;
};

/// Reflected multicast delegate binding (DObject, function name). Main-thread use only.
class FDynamicMulticastDelegate
{
public:
    void AddDynamic(DObject* object, const std::string& functionName)
    {
        if (!object || functionName.empty())
            return;

        FDynamicDelegateBinding binding;
        binding.m_objectHandle = object->GetGCHandle();
        binding.m_functionName   = functionName;
        m_bindings.push_back(std::move(binding));
    }

    void Clear() { m_bindings.clear(); }

    bool IsBound() const { return !m_bindings.empty(); }

    size_t GetBindingCount() const { return m_bindings.size(); }

    const std::vector<FDynamicDelegateBinding>& GetBindings() const { return m_bindings; }

    void BroadcastWithParams(void* params, uint32_t expectedParamCount) const
    {
        auto& registry = GetDObjectRegistry();
        for (const FDynamicDelegateBinding& binding : m_bindings)
        {
            DObject* object = registry.Resolve(binding.m_objectHandle);
            if (!object)
                continue;

            DFunction* function = object->GetClass()->FindFunctionByName(binding.m_functionName);
            if (!function)
                continue;

            if (function->HasReturnValue())
                continue;

            if (function->GetNumParams() != expectedParamCount)
                continue;

            function->Invoke(object, params);
        }
    }

    void Broadcast() const
    {
        BroadcastWithParams(nullptr, 0);
    }

private:
    std::vector<FDynamicDelegateBinding> m_bindings;
};

/// Reflected single-cast dynamic delegate. Main-thread use only.
class FDynamicDelegate : public FDynamicMulticastDelegate
{
public:
    using FDynamicMulticastDelegate::AddDynamic;
    using FDynamicMulticastDelegate::Clear;
    using FDynamicMulticastDelegate::IsBound;
    using FDynamicMulticastDelegate::GetBindingCount;
    using FDynamicMulticastDelegate::GetBindings;

    void ExecuteWithParams(void* params, uint32_t expectedParamCount) const
    {
        auto& registry = GetDObjectRegistry();
        for (const FDynamicDelegateBinding& binding : GetBindings())
        {
            DObject* object = registry.Resolve(binding.m_objectHandle);
            if (!object)
                return;

            DFunction* function = object->GetClass()->FindFunctionByName(binding.m_functionName);
            if (!function)
                return;

            if (function->HasReturnValue())
                return;

            if (function->GetNumParams() != expectedParamCount)
                return;

            function->Invoke(object, params);
            return;
        }
    }

    void Execute() const
    {
        ExecuteWithParams(nullptr, 0);
    }
};

DELTA_ENGINE_NS_END
