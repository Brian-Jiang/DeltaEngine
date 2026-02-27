#include "Reflection/ReflectionRegistry.h"

#include "Reflection/DClass.h"

#include <iostream>

using namespace DeltaEngine;

void ReflectionRegistry::RegisterDClass(DClass* cls)
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

void ReflectionRegistry::FinalizeRegistration()
{
    for (auto& pair : m_classMap)
    {
        const std::string& name = pair.first;
        DClass* cls = pair.second;
        const std::string& superName = cls->GetSuperName();
        if (superName.empty())
        {
            continue;
        }

        DClass* super = FindClassByName(superName);
        if (super)
        {
            cls->SetSuper(super);
        }
        else
        {
            std::cerr << "WARNING: Reflection superclass '" << superName
                      << "' for '" << name << "' not found in ReflectionRegistry.\n";
        }
    }
}

DClass* ReflectionRegistry::FindClassByName(const std::string& name) const
{
    auto it = m_classMap.find(name);
    if (it != m_classMap.end())
    {
        return it->second;
    }

    return nullptr;
}

DObject* ReflectionRegistry::CreateObject(const std::string& className) const
{
    DClass* cls = FindClassByName(className);
    if (cls)
    {
        void* memory = operator new(cls->GetClassSize(), std::align_val_t(cls->GetMinAlignment()));
        cls->ConstructObject(memory);
        return static_cast<DObject*>(memory);
    }

    return nullptr;
}

//DClass* ReflectionRegistry::FindClassByName(const std::string& name) const
//{
//    return nullptr;
// }

ReflectionRegistry& DeltaEngine::GetReflectionRegistry()
{
    static ReflectionRegistry registry;
    return registry;
}
