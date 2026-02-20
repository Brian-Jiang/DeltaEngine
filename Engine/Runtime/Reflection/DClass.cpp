#include "Runtime/Reflection/DClass.h"

using namespace DeltaEngine;

DClass::DClass(std::string name,
               DClass* super,
               size_t classSize,
               size_t minAlignment,
               void (*constructFn)(void* address),
               void (*destructFn)(void* address),
               void (*copyFn)(void* dest, const void* src),
               DObject* classDefaultObject)
    : m_name(std::move(name))
    , m_super(super)
    , m_properties(nullptr)
    , m_ownProperties(nullptr)
    , m_classSize(classSize)
    , m_minAlignment(minAlignment)
    , m_constructFn(constructFn)
    , m_destructFn(destructFn)
    , m_copyFn(copyFn)
    , m_classDefaultObject(classDefaultObject)
{
}
