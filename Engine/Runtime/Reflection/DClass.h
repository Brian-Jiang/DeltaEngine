#pragma once

#include "EngineIncludes.h"

#include <string>

DELTA_ENGINE_NS_BEGIN

class DObject;
class DProperty;
class ReflectionRegistry;

class DClass
{
    friend class ReflectionRegistry;

public:
    DClass(std::string name,
           DClass* super,
           size_t classSize,
           size_t minAlignment,
           void (*constructFn)(void* address),
           void (*destructFn)(void* address),
           void (*copyFn)(void* dest, const void* src),
           DObject* classDefaultObject
    );

    void AddProperty(DProperty* property);
    DProperty* FindPropertyByName(std::string name) const;
    bool IsChildOf(const DClass* other) const;


private:
    std::string m_name;
    DClass* m_super;
    DProperty* m_properties;
    DProperty* m_ownProperties;
    size_t m_classSize;
    size_t m_minAlignment;
    void (*m_constructFn)(void* address);
    void (*m_destructFn)(void* address);
    void (*m_copyFn)(void* dest, const void* src);
    DObject* m_classDefaultObject;
};

DELTA_ENGINE_NS_END
