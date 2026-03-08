#include "Assets/DPrimaryAsset.h"

#include "Reflection/DClass.h"
#include "Reflection/DProperty.h"
#include "Reflection/ReflectionRegistry.h"
#include "Serialization/AssetArchive.h"

#include <algorithm>

using namespace DeltaEngine;

void DPrimaryAsset::AddObject(std::shared_ptr<DObject> obj)
{
    obj->SetOwningAsset(this);
    m_objects.push_back(std::move(obj));
    MarkDirty();
}

void DPrimaryAsset::RemoveObject(const ObjectId& id)
{
    auto it = std::remove_if(m_objects.begin(), m_objects.end(),
        [&](const auto& obj) { return obj->GetObjectId() == id; });
    if (it != m_objects.end())
    {
        for (auto jt = it; jt != m_objects.end(); ++jt)
            (*jt)->SetOwningAsset(nullptr);
        m_objects.erase(it, m_objects.end());
        MarkDirty();
    }
}

DObject* DPrimaryAsset::FindObject(const ObjectId& id) const
{
    for (auto& obj : m_objects)
    {
        if (obj->GetObjectId() == id)
            return obj.get();
    }
    return nullptr;
}

const std::vector<std::shared_ptr<DObject>>& DPrimaryAsset::GetObjects() const
{
    return m_objects;
}

void DPrimaryAsset::SerializeHeader(AssetArchive& ar)
{
    if (ar.IsSaving())
    {
        std::string magic = "DLTA";
        ar.Serialize("magic", magic);

        int version = static_cast<int>(m_header.m_fileVersion);
        ar.Serialize("version", version);

        ar.Serialize("className", m_header.m_className);

        UUID aid = m_header.m_persistentId;
        ar.Serialize("assetId", aid);
    }
    else
    {
        std::string magic;
        ar.Serialize("magic", magic);
        m_header.m_magic = (magic == "DLTA") ? 0x444C5441 : 0;

        int version = 0;
        ar.Serialize("version", version);
        m_header.m_fileVersion = static_cast<uint32_t>(version);

        ar.Serialize("className", m_header.m_className);

        ar.Serialize("assetId", m_header.m_persistentId);
    }
}

void DPrimaryAsset::SerializeBody(AssetArchive& ar)
{
    if (ar.IsSaving())
    {
        ar.BeginArray("objects", m_objects.size());
        for (auto& obj : m_objects)
        {
            DClass* cls = obj->GetClass();
            std::string className = cls->GetName();
            ar.BeginObject(className);

            UUID oid = obj->GetObjectId();
            ar.Serialize("_objectId", oid);

            cls->Serialize(ar, *obj);

            ar.EndObject();
        }
        ar.EndArray();
    }
    else
    {
        DeserializeBody(ar);
    }
}

void DPrimaryAsset::DeserializeBody(AssetArchive& ar)
{
    size_t count = ar.BeginArrayLoad("objects");
    for (size_t i = 0; i < count; ++i)
    {
        std::string className = ar.BeginObjectLoad();

        DClass* dclass = GetReflectionRegistry().FindClassByName(className);
        if (!dclass)
        {
            ar.EndObject();
            continue;
        }

        DObject* raw = GetReflectionRegistry().CreateObject(className);
        if (!raw)
        {
            ar.EndObject();
            continue;
        }

        std::shared_ptr<DObject> obj(raw, [](DObject* p)
        {
            GetReflectionRegistry().DestroyObject(p);
        });

        ObjectId oid;
        ar.Serialize("_objectId", oid);
        obj->SetObjectId(oid);
        obj->SetOwningAsset(this);

        dclass->Serialize(ar, *obj);

        ar.EndObject();
        m_objects.push_back(std::move(obj));
    }
    ar.EndArray();
}

std::vector<ScriptPointer> DPrimaryAsset::CollectExternalReferences() const
{
    std::vector<ScriptPointer> refs;
    for (auto& obj : m_objects)
    {
        DClass* cls = obj->GetClass();
        for (DProperty* prop = cls->GetProperties(); prop; prop = prop->GetNext())
        {
            auto* ptrProp = dynamic_cast<DObjectPtrPropertyBase*>(prop);
            if (!ptrProp)
                continue;

            ScriptPointer sp = ptrProp->GetUnresolvedPointer(obj.get());
            if (!sp.IsNull() && sp.IsExternal(GetAssetId()))
                refs.push_back(sp);
        }
    }
    return refs;
}
