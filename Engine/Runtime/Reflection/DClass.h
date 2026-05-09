#pragma once

#include "EngineIncludes.h"

#include "Runtime/Reflection/DStruct.h"

#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

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

    /// Adds a reflected function declared on this class. Multiple overloads with the same name are supported.
    void AddFunction(DFunction* function);
    /// Returns the first reflected function with the given name on this class or one of its bases.
    DELTAENGINE_API DFunction* FindFunctionByName(const std::string& name) const;
    /// Returns all overloads with the given name declared directly on this class (no base walk).
    DELTAENGINE_API std::vector<DFunction*> FindOverloads(const std::string& name) const;
    /// Returns the overload whose parameter types exactly match `paramTypes`, searching this class and its bases.
    DELTAENGINE_API DFunction* FindFunction(const std::string& name,
                                            std::span<const std::string_view> paramTypes) const;
    /// Returns a flattened view of every function declared directly on this class.
    DELTAENGINE_API std::vector<DFunction*> GetFunctions() const;

    /// Returns true when this class is the same as or derived from `other`.
    DELTAENGINE_API bool IsChildOf(const DClass* other) const;
    /// Returns true when the class cannot be instantiated through reflection.
    DELTAENGINE_API bool IsAbstract() const;

    DELTAENGINE_API void ConstructObject(void* address) const;
    DELTAENGINE_API void DestroyObject(void* address) const;
    DELTAENGINE_API void CopyObject(void* dest, const void* src) const;

private:
    void (*m_constructFn)(void* address);
    void (*m_destructFn)(void* address);
    void (*m_copyFn)(void* dest, const void* src);
    DObject* m_classDefaultObject;
    bool m_abstract;

    std::unordered_map<std::string, std::vector<DFunction*>> m_functions;
};

DELTA_ENGINE_NS_END
