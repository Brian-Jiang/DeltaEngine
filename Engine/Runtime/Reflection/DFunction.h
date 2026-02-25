#pragma once

#include "EngineIncludes.h"

#include <string>
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

    DFunction(std::string name,
              NativeFn nativeFn,
              uint32_t numParams,
              uint32_t totalSize,
              uint32_t returnValueOffset);

    void Invoke(DObject* context, void* params) const;

    void AddParam(DProperty* param);
    void SetReturnProperty(DProperty* prop);

    const std::string& GetName() const;
    DClass* GetDeclaringClass() const;
    uint32_t GetNumParams() const;
    uint32_t GetTotalSize() const;
    uint32_t GetReturnValueOffset() const;
    const std::vector<DProperty*>& GetParams() const;
    DProperty* GetReturnProperty() const;
    bool HasReturnValue() const;

private:
    std::string m_name;
    DClass* m_declaringClass = nullptr;
    uint32_t m_numParams;
    uint32_t m_totalSize;
    uint32_t m_returnValueOffset;
    NativeFn m_nativeFn;

    std::vector<DProperty*> m_params;
    DProperty* m_returnProperty = nullptr;
};

DELTA_ENGINE_NS_END
