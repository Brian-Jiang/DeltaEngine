#pragma once

#include "EngineIncludes.h"

#include "Runtime/Core/DHandle.h"
#include "Runtime/Core/GC/DObjectGCTypes.h"
#include "Runtime/Core/UUID.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

#include "DObject.generated.h"

DELTA_ENGINE_NS_BEGIN

class DClass;
class DProperty;
class DPrimaryAsset;

DCLASS()
class DObject
{
    friend class DeltaEngine::Reflection::Private::ReflectionRegister_DObject;

public:
    virtual DClass* GetClass() const
    {
        return GetReflectionRegistry().FindClassByName("DObject");
    }

    friend class DClass;

public:
    DELTAENGINE_API DObject();
    DELTAENGINE_API virtual ~DObject();

    void SetHandle(const DHandle& handle) { m_handle = handle; }

    ObjectId GetObjectId() const { return m_objectId; }
    void SetObjectId(const ObjectId& id) { m_objectId = id; }

    DPrimaryAsset* GetOwningAsset() const { return m_owningAsset; }
    void SetOwningAsset(DPrimaryAsset* asset) { m_owningAsset = asset; }

    bool HasOwningAsset() const { return m_owningAsset != nullptr; }

    DELTAENGINE_API void MarkDirty();

    DObjectHandle GetGCHandle() const { return { m_gcSlotIndex, m_gcSlotVersion }; }
    void SetGCHandle(const DObjectHandle& handle)
    {
        m_gcSlotIndex   = handle.m_slotIndex;
        m_gcSlotVersion = handle.m_version;
    }

    EGCMarkColor GetGCMarkColor() const { return m_gcMarkColor; }
    void SetGCMarkColor(EGCMarkColor color) { m_gcMarkColor = color; }

    virtual void PostEditChangeProperty(const DProperty* prop) { (void)prop; }
    virtual void PostRestore() {}

    virtual void BeginDestroy() {}
    virtual bool IsReadyForFinishDestroy() { return true; }
    virtual void FinishDestroy() {}

private:
    DHandle        m_handle;
    ObjectId       m_objectId;
    DPrimaryAsset* m_owningAsset = nullptr;

    GCSlotIndex    m_gcSlotIndex   = kInvalidGCSlot;
    GCSlotVersion  m_gcSlotVersion = 0;
    EGCMarkColor   m_gcMarkColor   = EGCMarkColor::White;

};

template <typename T>
concept DObjectDerived = std::derived_from<std::remove_pointer_t<T>, DObject>;

DELTA_ENGINE_NS_END