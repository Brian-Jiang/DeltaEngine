#pragma once

#include "EngineIncludes.h"

#include <unordered_map>
#include <string>

//export module DeltaEngine.Runtime.Reflection;
//
//import <unordered_map>

DELTA_ENGINE_NS_BEGIN

class DClass;

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
    void RegisterClass(DClass* dclass);
    DClass* FindClassByName(const std::string& name) const;

private:
    std::unordered_map<std::string, DClass*> m_classMap;
};

DELTA_ENGINE_NS_END
