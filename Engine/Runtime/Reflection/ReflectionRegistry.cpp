#include "Reflection/ReflectionRegistry.h"

#include "Reflection/DStruct.h"
#include "Reflection/DClass.h"
#include "Core/DObject.h"
#include "Core/DHandle.h"

#include <iostream>

using namespace DeltaEngine;

void ReflectionRegistry::RegisterDStruct(DStruct* dstruct)
{
    const std::string& name = dstruct->GetName();
    if (m_structMap.find(name) == m_structMap.end())
    {
        m_structMap[name] = dstruct;
    }
}

void ReflectionRegistry::RegisterDClass(DClass* cls)
{
    const std::string& name = cls->GetName();
    if (m_classMap.find(name) == m_classMap.end())
    {
        m_classMap[name] = cls;
    }
}

void ReflectionRegistry::FinalizeRegistration()
{
    for (auto& [name, dstruct] : m_structMap)
    {
        const std::string& superName = dstruct->GetSuperName();
        if (superName.empty())
            continue;

        DStruct* super = FindStructByName(superName);
        if (!super)
        {
            DClass* superClass = FindClassByName(superName);
            super = superClass;
        }
        if (super)
        {
            dstruct->SetSuper(super);
        }
        else
        {
            std::cerr << "WARNING: Reflection superclass '" << superName
                      << "' for struct '" << name << "' not found in ReflectionRegistry.\n";
        }
    }

    for (auto& [name, cls] : m_classMap)
    {
        const std::string& superName = cls->GetSuperName();
        if (superName.empty())
            continue;

        DClass* superClass = FindClassByName(superName);
        if (superClass)
        {
            cls->SetSuper(superClass);
        }
        else
        {
            DStruct* superStruct = FindStructByName(superName);
            if (superStruct)
            {
                cls->SetSuper(superStruct);
            }
            else
            {
                std::cerr << "WARNING: Reflection superclass '" << superName
                          << "' for '" << name << "' not found in ReflectionRegistry.\n";
            }
        }
    }
}

DStruct* ReflectionRegistry::FindStructByName(const std::string& name) const
{
    auto it = m_structMap.find(name);
    if (it != m_structMap.end())
        return it->second;
    return nullptr;
}

DClass* ReflectionRegistry::FindClassByName(const std::string& name) const
{
    auto it = m_classMap.find(name);
    if (it != m_classMap.end())
        return it->second;
    return nullptr;
}

const std::unordered_map<std::string, DClass*>& ReflectionRegistry::GetAllClasses() const
{
    return m_classMap;
}

void ReflectionRegistry::DestroyObject(DObject* obj) const
{
    if (!obj)
        return;

    DClass* cls = obj->GetClass();
    if (cls)
    {
        cls->DestroyObject(obj);
        operator delete(obj, std::align_val_t(cls->GetMinAlignment()));
    }
}

DObject* ReflectionRegistry::CreateObject(const std::string& className) const
{
    DClass* cls = FindClassByName(className);
    if (cls)
    {
        if (cls->IsAbstract())
        {
            std::cerr << "WARNING: Cannot create instance of abstract class '" << className << "'.\n";
            return nullptr;
        }

        void* memory = operator new(cls->GetStructSize(), std::align_val_t(cls->GetMinAlignment()));
        cls->ConstructObject(memory);
        DHandle handle {};
        handle.m_ptr = memory;
        DObject* obj = static_cast<DObject*>(memory);
        obj->SetHandle(handle);
        obj->SetObjectId(ObjectId::Generate());
        obj->SetOwningAsset(nullptr);
        return obj;
    }

    return nullptr;
}

ReflectionRegistry& DeltaEngine::GetReflectionRegistry()
{
    static ReflectionRegistry registry;
    return registry;
}
