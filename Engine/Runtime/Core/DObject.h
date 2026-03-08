#pragma once

#include "EngineIncludes.h"
#include "Core/DHandle.h"
#include "Core/UUID.h"
#include "Reflection/ReflectionRegistry.h"

#include "DObject.generated.h"

DELTA_ENGINE_NS_BEGIN

class DClass;
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

    DELTAENGINE_API void MarkDirty();

private:
    DHandle        m_handle;
    ObjectId       m_objectId;
    DPrimaryAsset* m_owningAsset = nullptr;

};

template <typename T>
concept DObjectDerived = std::derived_from<std::remove_pointer_t<T>, DObject>;

DELTA_ENGINE_NS_END