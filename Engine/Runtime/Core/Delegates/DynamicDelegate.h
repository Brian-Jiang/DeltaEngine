#pragma once

#include "EngineIncludes.h"

#include "Runtime/Core/DObject.h"
#include "Runtime/Core/GC/DObjectRegistry.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DFunction.h"

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

    void Broadcast() const
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

            function->Invoke(object, nullptr);
        }
    }

private:
    std::vector<FDynamicDelegateBinding> m_bindings;
};

DELTA_ENGINE_NS_END
