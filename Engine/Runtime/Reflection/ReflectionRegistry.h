#pragma once

#include "EngineIncludes.h"

#include <unordered_map>
#include <string>

//export module DeltaEngine.Runtime.Reflection;
//
//import <unordered_map>

DELTA_ENGINE_NS_BEGIN

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
    void RegisterDClass(DClass* dclass);
    void FinalizeRegistration();
    DClass* FindClassByName(const std::string& name) const;
    DObject* CreateObject(const std::string& className) const;

    template <typename T>
    T* CreateObject(const std::string& className) const
    {
        DObject* obj = CreateObject(className);
        return static_cast<T*>(obj);
    }

private:
    std::unordered_map<std::string, DClass*> m_classMap;
};

extern ReflectionRegistry& GetReflectionRegistry();

DELTA_ENGINE_NS_END
