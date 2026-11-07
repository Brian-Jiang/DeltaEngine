#include "Runtime/Reflection/ReflectionRegistry.h"

#include "Runtime/Core/DHandle.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Core/GC/DObjectRegistry.h"
#include "Runtime/Core/GC/GCManager.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DEnum.h"
#include "Runtime/Reflection/DStruct.h"
#include "Runtime/Logging/LogChannels.h"

using namespace DeltaEngine;

void ReflectionRegistry::RegisterDStruct(DStruct* dstruct)
{
    DELTA_VERIFY(dstruct != nullptr);
    const std::string& name = dstruct->GetName();
    DELTA_VERIFY(!name.empty());

    auto [it, inserted] = m_structMap.try_emplace(name, dstruct);
    if (!inserted)
        DLOG(LogReflection, ELogLevel::Warning,
             "Ignored duplicate Reflection struct registration name='{}'; expected single definition (keeping first)",
             name);
}

void ReflectionRegistry::RegisterDClass(DClass* cls)
{
    DELTA_VERIFY(cls != nullptr);
    const std::string& name = cls->GetName();
    DELTA_VERIFY(!name.empty());

    auto [it, inserted] = m_classMap.try_emplace(name, cls);
    if (!inserted)
        DLOG(LogReflection, ELogLevel::Warning,
             "Ignored duplicate Reflection class registration name='{}'; expected single definition (keeping first)",
             name);
}

void ReflectionRegistry::RegisterDEnum(DEnum* denum)
{
    DELTA_VERIFY(denum != nullptr);
    const std::string& name = denum->GetName();
    DELTA_VERIFY(!name.empty());

    auto [it, inserted] = m_enumMap.try_emplace(name, denum);
    if (!inserted)
        DLOG(LogReflection, ELogLevel::Warning,
             "Ignored duplicate Reflection enum registration name='{}'; expected single definition (keeping first)",
             name);
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
            DLOG(LogReflection, ELogLevel::Error,
                 "FinalizeRegistration failed for struct '{}': superclass '{}' missing; register base before derived",
                 name, superName);
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
                DLOG(LogReflection, ELogLevel::Error,
                     "FinalizeRegistration failed for class '{}': superclass '{}' missing; register base before derived",
                     name, superName);
            }
        }
    }

    for (auto& [name, dstruct] : m_structMap)
        dstruct->RebuildHierarchyChain();
    for (auto& [name, cls] : m_classMap)
        cls->RebuildHierarchyChain();
}

DStruct* ReflectionRegistry::FindStructByName(const std::string& name) const
{
    if (name.empty())
    {
        DLOG(LogReflection, ELogLevel::Warning,
             "FindStructByName failed: requested name was empty (expected non-empty)");
        return nullptr;
    }

    auto it = m_structMap.find(name);
    if (it != m_structMap.end())
        return it->second;

    return nullptr;
}

DClass* ReflectionRegistry::FindClassByName(const std::string& name) const
{
    if (name.empty())
    {
        DLOG(LogReflection, ELogLevel::Warning,
             "FindClassByName failed: requested name was empty (expected non-empty)");
        return nullptr;
    }

    auto it = m_classMap.find(name);
    if (it != m_classMap.end())
        return it->second;

    return nullptr;
}

DEnum* ReflectionRegistry::FindEnumByName(const std::string& name) const
{
    if (name.empty())
    {
        DLOG(LogReflection, ELogLevel::Warning,
             "FindEnumByName failed: requested name was empty (expected non-empty)");
        return nullptr;
    }

    auto it = m_enumMap.find(name);
    if (it != m_enumMap.end())
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
    {
        DLOG(LogReflection, ELogLevel::Verbose, "DestroyObject no-op for null object pointer");
        return;
    }

    DClass* cls = obj->GetClass();
    if (!cls)
    {
        DLOG(LogReflection, ELogLevel::Error,
             "DestroyObject aborted: object at {} has null GetClass(); expected registered DClass",
             reinterpret_cast<void*>(obj));
        return;
    }

    const DObjectHandle gcHandle = obj->GetGCHandle();

    cls->DestroyObject(obj);
    operator delete(obj, std::align_val_t(cls->GetMinAlignment()));

    GetDObjectRegistry().FreeSlot(gcHandle);
}

DObject* ReflectionRegistry::CreateObject(const std::string& className) const
{
    if (className.empty())
    {
        DLOG(LogReflection, ELogLevel::Warning,
             "CreateObject failed: class name empty (expected non-empty)");
        return nullptr;
    }

    DClass* cls = FindClassByName(className);
    if (!cls)
    {
        DLOG(LogReflection, ELogLevel::Warning,
             "CreateObject failed: class '{}' unregistered (expected ReflectionRegistry.RegisterDClass output)",
             className);
        return nullptr;
    }

    if (cls->IsAbstract())
    {
        DLOG(LogReflection, ELogLevel::Warning,
             "CreateObject refused: '{}' is abstract (expected concrete subclass name)", className);
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

    GetDObjectRegistry().RegisterObject(obj);
    // Objects created mid-mark are colored Black so the in-progress traversal treats
    // them as reachable; otherwise they start White (unreachable until marked).
    obj->SetGCMarkColor(GetGCManager().IsMarking() ? EGCMarkColor::Black
                                                   : EGCMarkColor::White);

    return obj;
}

ReflectionRegistry& DeltaEngine::GetReflectionRegistry()
{
    static ReflectionRegistry registry;
    return registry;
}
