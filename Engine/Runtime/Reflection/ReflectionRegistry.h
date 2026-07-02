#pragma once

#include "EngineIncludes.h"

#include <unordered_map>
#include <string>

DELTA_ENGINE_NS_BEGIN

class DStruct;
class DClass;
class DEnum;
class DObject;

class ReflectionRegistration
{
public:
    /// Registers a reflected type during static initialization.
    ReflectionRegistration(void (*registerFn)())
    {
        registerFn();
    }
};

class ReflectionRegistry
{

public:
    /// Adds a reflected struct if it has not been registered yet.
    void RegisterDStruct(DStruct* dstruct);
    /// Adds a reflected class if it has not been registered yet.
    void RegisterDClass(DClass* dclass);
    /// Adds a reflected enum if it has not been registered yet.
    DELTAENGINE_API void RegisterDEnum(DEnum* denum);
    /// Resolves reflected base-type links after static registration completes.
    DELTAENGINE_API void FinalizeRegistration();

    /// Finds a reflected struct by name.
    DELTAENGINE_API DStruct* FindStructByName(const std::string& name) const;
    /// Finds a reflected class by name.
    DELTAENGINE_API DClass* FindClassByName(const std::string& name) const;
    /// Finds a reflected enum by name.
    DELTAENGINE_API DEnum* FindEnumByName(const std::string& name) const;
    /// Returns the registered reflected classes keyed by class name.
    DELTAENGINE_API const std::unordered_map<std::string, DClass*>& GetAllClasses() const;

    /// Creates an object instance for the named reflected class.
    DELTAENGINE_API DObject* CreateObject(const std::string& className) const;

    template <typename T>
    T* CreateObject(const std::string& className) const
    {
        DObject* obj = CreateObject(className);
        return static_cast<T*>(obj);
    }

    /// Destroys an object created through the reflection registry.
    DELTAENGINE_API void DestroyObject(DObject* obj) const;

private:
    std::unordered_map<std::string, DStruct*> m_structMap;
    std::unordered_map<std::string, DClass*> m_classMap;
    std::unordered_map<std::string, DEnum*> m_enumMap;
};

/// Returns the global reflection registry instance.
DELTAENGINE_API extern ReflectionRegistry& GetReflectionRegistry();

DELTA_ENGINE_NS_END
