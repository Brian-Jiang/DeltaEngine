#pragma once

#include "EngineIncludes.h"

#include <unordered_map>
#include <string>

DELTA_ENGINE_NS_BEGIN

class DStruct;
class DClass;
class DObject;

class ReflectionRegistration
{
public:
    ReflectionRegistration(void (*registerFn)())
    {
        registerFn();
    }
};

class ReflectionRegistry
{

public:
    void RegisterDStruct(DStruct* dstruct);
    void RegisterDClass(DClass* dclass);
    void FinalizeRegistration();

    DELTAENGINE_API DStruct* FindStructByName(const std::string& name) const;
    DELTAENGINE_API DClass* FindClassByName(const std::string& name) const;
    DELTAENGINE_API const std::unordered_map<std::string, DClass*>& GetAllClasses() const;

    DObject* CreateObject(const std::string& className) const;

    template <typename T>
    T* CreateObject(const std::string& className) const
    {
        DObject* obj = CreateObject(className);
        return static_cast<T*>(obj);
    }

    void DestroyObject(DObject* obj) const;

private:
    std::unordered_map<std::string, DStruct*> m_structMap;
    std::unordered_map<std::string, DClass*> m_classMap;
};

DELTAENGINE_API extern ReflectionRegistry& GetReflectionRegistry();

DELTA_ENGINE_NS_END
