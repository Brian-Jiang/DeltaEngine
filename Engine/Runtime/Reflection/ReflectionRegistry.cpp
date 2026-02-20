#include "Reflection/ReflectionRegistry.h"

#include "Reflection/DClass.h"

using namespace DeltaEngine;

void ReflectionRegistry::RegisterClass(DClass* cls)
{
    std::string& name = cls->m_name;
    if (m_classMap.find(name) == m_classMap.end())
    {
        m_classMap[name] = cls;
    }
    else
    {
        // Handle duplicate class registration if necessary
    }
}

//DClass* ReflectionRegistry::FindClassByName(const std::string& name) const
//{
//    return nullptr;
//}