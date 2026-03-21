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
    /// Describes a reflected object type and its construction hooks.
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

    /// Adds a reflected function declared on this class.
    void AddFunction(DFunction* function);
    /// Finds a reflected function by name on this class or one of its bases.
    DELTAENGINE_API DFunction* FindFunctionByName(const std::string& name) const;

    /// Returns true when this class is the same as or derived from `other`.
    DELTAENGINE_API bool IsChildOf(const DClass* other) const;
    /// Returns true when the class cannot be instantiated through reflection.
    DELTAENGINE_API bool IsAbstract() const;

    /// Constructs an instance in caller-provided storage.
    void ConstructObject(void* address) const;
    /// Destroys an instance stored at the provided address.
    void DestroyObject(void* address) const;
    /// Copy-constructs an instance into caller-provided storage.
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
