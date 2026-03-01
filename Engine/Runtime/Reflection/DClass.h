#pragma once

#include "EngineIncludes.h"

#include "Runtime/Reflection/DStruct.h"

#include <string>
#include <unordered_map>

DELTA_ENGINE_NS_BEGIN

class DObject;
class DProperty;
class DFunction;
class ReflectionRegistry;

class DClass : public DStruct
{
    friend class ReflectionRegistry;

public:
    DClass(std::string name,
           std::string superName,
           size_t classSize,
           size_t minAlignment,
           void (*constructFn)(void* address),
           void (*destructFn)(void* address),
           void (*copyFn)(void* dest, const void* src),
           DObject* classDefaultObject,
           bool isAbstract = false
    );

    void AddFunction(DFunction* function);
    DFunction* FindFunctionByName(const std::string& name) const;

    bool IsChildOf(const DClass* other) const;
    bool IsAbstract() const;

    void ConstructObject(void* address) const;
    void DestroyObject(void* address) const;
    void CopyObject(void* dest, const void* src) const;

private:
    void (*m_constructFn)(void* address);
    void (*m_destructFn)(void* address);
    void (*m_copyFn)(void* dest, const void* src);
    DObject* m_classDefaultObject;
    bool m_abstract;

    std::unordered_map<std::string, DFunction*> m_functions;
};

DELTA_ENGINE_NS_END
