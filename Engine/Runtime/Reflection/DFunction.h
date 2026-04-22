#pragma once

#include "EngineIncludes.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

DELTA_ENGINE_NS_BEGIN

class DClass;
class DObject;
class DProperty;

class DFunction
{
    friend class DClass;

public:
    using NativeFn = void(*)(DObject*, void*);

    /// Describes a reflected function and its generated parameter layout.
    DFunction(std::string name,
              NativeFn nativeFn,
              uint32_t numParams,
              uint32_t totalSize,
              uint32_t returnValueOffset);

    /// Invokes the function on the provided object using the generated params block.
    DELTAENGINE_API void Invoke(DObject* context, void* params) const;

    /// Adds a reflected input parameter in declaration order.
    void AddParam(DProperty* param);
    /// Sets the reflected return property when the function returns a value.
    void SetReturnProperty(DProperty* prop);

    DELTAENGINE_API const std::string& GetName() const;
    DELTAENGINE_API DClass* GetDeclaringClass() const;
    DELTAENGINE_API uint32_t GetNumParams() const;
    DELTAENGINE_API uint32_t GetTotalSize() const;
    DELTAENGINE_API uint32_t GetReturnValueOffset() const;
    DELTAENGINE_API const std::vector<DProperty*>& GetParams() const;
    DELTAENGINE_API DProperty* GetReturnProperty() const;
    DELTAENGINE_API bool HasReturnValue() const;

    void SetMetadata(std::unordered_map<std::string, std::string> metadata)
    {
        m_metadata = std::move(metadata);
    }

    bool HasMeta(const std::string& key) const
    {
        return m_metadata.count(key) > 0;
    }

    DELTAENGINE_API std::string GetMeta(const std::string& key, const std::string& defaultVal = "") const;

private:
    std::string m_name;
    DClass* m_declaringClass = nullptr;
    uint32_t m_numParams;
    uint32_t m_totalSize;
    uint32_t m_returnValueOffset;
    NativeFn m_nativeFn;

    std::vector<DProperty*> m_params;
    DProperty* m_returnProperty = nullptr;
    std::unordered_map<std::string, std::string> m_metadata;
};

DELTA_ENGINE_NS_END
