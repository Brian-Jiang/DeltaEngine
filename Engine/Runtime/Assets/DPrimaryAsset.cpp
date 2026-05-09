#include "Runtime/Assets/DPrimaryAsset.h"

#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DObjectReferenceTraversal.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/DBulkDataProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Serialization/AssetArchive.h"
#include "Runtime/Serialization/TBulkData.h"

#include <algorithm>

using namespace DeltaEngine;

namespace
{
constexpr uint32_t kSupportedPrimaryAssetFileVersion = 1;

void EnsureDynamicMetaShape(nlohmann::json& meta, bool warn)
{
    if (!meta.is_object())
    {
        if (warn)
        {
            DLOG(LogAsset, ELogLevel::Warning,
                 "Dynamic asset meta is not an object (was '{}'); replacing with empty defaults",
                 meta.type_name());
        }
        meta = nlohmann::json::object();
    }

    auto descIt = meta.find("desc");
    if (descIt == meta.end())
    {
        if (warn)
        {
            DLOG(LogAsset, ELogLevel::Warning,
                 "Dynamic asset meta missing 'desc'; backfilling empty string");
        }
        meta["desc"] = std::string();
    }
    else if (!descIt->is_string())
    {
        if (warn)
        {
            DLOG(LogAsset, ELogLevel::Warning,
                 "Dynamic asset meta 'desc' has wrong type '{}'; replacing with empty string",
                 descIt->type_name());
        }
        *descIt = std::string();
    }

    auto tagsIt = meta.find("tags");
    if (tagsIt == meta.end())
    {
        if (warn)
        {
            DLOG(LogAsset, ELogLevel::Warning,
                 "Dynamic asset meta missing 'tags'; backfilling empty array");
        }
        meta["tags"] = nlohmann::json::array();
    }
    else if (!tagsIt->is_array())
    {
        if (warn)
        {
            DLOG(LogAsset, ELogLevel::Warning,
                 "Dynamic asset meta 'tags' has wrong type '{}'; replacing with empty array",
                 tagsIt->type_name());
        }
        *tagsIt = nlohmann::json::array();
    }
    else
    {
        nlohmann::json cleaned = nlohmann::json::array();
        bool dropped = false;
        for (auto& entry : *tagsIt)
        {
            if (entry.is_string())
                cleaned.push_back(entry);
            else
                dropped = true;
        }
        if (dropped)
        {
            if (warn)
            {
                DLOG(LogAsset, ELogLevel::Warning,
                     "Dynamic asset meta 'tags' contained non-string entries; dropped them");
            }
            *tagsIt = std::move(cleaned);
        }
    }
}
}

void DPrimaryAsset::AddObject(DObject* obj)
{
    DELTA_VERIFY_MSG(obj != nullptr, "DPrimaryAsset::AddObject requires non-null object");
    obj->SetOwningAsset(this);
    m_objects.push_back(obj);
    MarkDirty();
}

void DPrimaryAsset::RemoveObject(const ObjectId &id)
{
    auto it =
        std::find_if(m_objects.begin(), m_objects.end(), [&](const auto &obj) {
            DELTA_VERIFY_MSG(obj != nullptr, "DPrimaryAsset::RemoveObject search encountered null object slot");
            return obj->GetObjectId() == id;
        });
    if (it != m_objects.end())
    {
        DObject* removed = *it;
        DELTA_VERIFY_MSG(removed != nullptr, "DPrimaryAsset::RemoveObject encountered null slot");
        removed->SetOwningAsset(nullptr);
        m_objects.erase(it);
        MarkDirty();
    }
}

DObject* DPrimaryAsset::FindObject(const ObjectId& id) const
{
    for (auto* obj : m_objects)
    {
        DELTA_VERIFY_MSG(obj != nullptr, "DPrimaryAsset::FindObject encountered null object slot");
        if (obj->GetObjectId() == id)
            return obj;
    }
    return nullptr;
}

const std::vector<DObject*>& DPrimaryAsset::GetObjects() const
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
        if (magic != "DLTA")
        {
            DLOG(LogAsset,
                 ELogLevel::Warning,
                 "Primary asset header magic mismatch: got '{}' (expected '{}'); storing mismatched magic in header",
                 magic,
                 "DLTA");
        }
        m_header.m_magic = (magic == "DLTA") ? 0x444C5441 : 0;

        int version = 0;
        ar.Serialize("version", version);
        m_header.m_fileVersion = static_cast<uint32_t>(version);
        if (static_cast<uint32_t>(version) != kSupportedPrimaryAssetFileVersion)
        {
            DLOG(LogAsset,
                 ELogLevel::Warning,
                 "Primary asset file version {} unsupported (expected {}); deserialization may be unstable",
                 version,
                 kSupportedPrimaryAssetFileVersion);
        }
        DELTA_ENSURE_MSG(version >= 0, "Primary asset file version negative");

        ar.Serialize("className", m_header.m_className);

        ar.Serialize("assetId", m_header.m_persistentId);
    }
}

void DPrimaryAsset::SerializeBody(AssetArchive& ar)
{
    if (!ar.IsSaving())
    {
        DeserializeBody(ar);
        return;
    }

    ar.BeginArray("objects", m_objects.size());
    for (DObject* obj : m_objects)
    {
        DELTA_VERIFY_MSG(obj != nullptr, "DPrimaryAsset::SerializeBody encountered null object");
        DClass* cls = obj->GetClass();
        DELTA_VERIFY_MSG(cls != nullptr, "DPrimaryAsset::SerializeBody requires valid reflected class");
        std::string className = cls->GetName();
        ar.BeginObject(className);

        UUID oid = obj->GetObjectId();
        ar.Serialize("_objectId", oid);

        cls->Serialize(ar, *obj);

        ar.EndObject();
    }
    ar.EndArray();
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
            DLOG(LogAsset,
                 ELogLevel::Error,
                 "Skipping asset object {} / {}: class '{}' not registered in ReflectionRegistry",
                 i + 1,
                 count,
                 className);
            ar.EndObject();
            continue;
        }

        DObject* obj = GetReflectionRegistry().CreateObject(className);
        if (!obj)
        {
            DLOG(LogAsset,
                 ELogLevel::Error,
                 "Skipping asset object {} / {}: CreateObject failed for registered class '{}' "
                 "(expected successful reflected instantiation)",
                 i + 1,
                 count,
                 className);
            ar.EndObject();
            continue;
        }

        ObjectId oid;
        ar.Serialize("_objectId", oid);
        obj->SetObjectId(oid);
        obj->SetOwningAsset(this);

        dclass->Serialize(ar, *obj);

        ar.EndObject();
        m_objects.push_back(obj);
    }
    ar.EndArray();
}

std::vector<std::pair<DBulkDataProperty*, DObject*>> DPrimaryAsset::CollectBulkProperties() const
{
    std::vector<std::pair<DBulkDataProperty*, DObject*>> result;

    auto walkProps = [&](auto& self, DStruct* ds, DObject* obj) -> void {
        if (!ds) return;
        if (DStruct* parent = ds->GetSuper())
            self(self, parent, obj);
        std::vector<std::pair<DBulkDataProperty*, DObject*>> local;
        for (DProperty* prop = ds->GetOwnProperties(); prop; prop = prop->GetNext())
        {
            if (auto* bp = dynamic_cast<DBulkDataProperty*>(prop))
                local.emplace_back(bp, obj);
        }
        std::reverse(local.begin(), local.end());
        result.insert(result.end(), local.begin(), local.end());
    };

    for (auto* obj : m_objects)
    {
        DELTA_VERIFY_MSG(obj != nullptr, "DPrimaryAsset::CollectBulkProperties encountered null object");
        walkProps(walkProps, obj->GetClass(), obj);
    }

    return result;
}

void DPrimaryAsset::SerializeBulkData(AssetArchive& ar)
{
    auto bulkProps = CollectBulkProperties();

    if (ar.IsSaving())
    {
        uint32_t nextId = 0;
        for (auto& [prop, obj] : bulkProps)
        {
            DELTA_VERIFY_MSG(prop != nullptr && obj != nullptr, "SerializeBulkData invalid bulk property slot");
            auto* bulkData = static_cast<TBulkData*>(prop->GetValue(obj));
            DELTA_VERIFY_MSG(bulkData != nullptr, "SerializeBulkData missing TBulkData storage for property '{}'",
                             prop->GetName());
            bulkData->m_bulkId = nextId++;
        }
    }

    for (auto& [prop, obj] : bulkProps)
    {
        DELTA_VERIFY_MSG(prop != nullptr && obj != nullptr, "SerializeBulkData invalid bulk property slot");
        prop->SerializeBulkPayload(ar, obj);
    }
}

void DPrimaryAsset::SerializeMeta(AssetArchive& ar)
{
    nlohmann::json staticBlock = nlohmann::json::object();
    ar.Serialize("static", staticBlock);

    if (ar.IsSaving())
    {
        nlohmann::json dynamicOut = m_dynamicMeta;
        EnsureDynamicMetaShape(dynamicOut, /*warn=*/false);
        ar.Serialize("dynamic", dynamicOut);
    }
    else
    {
        ar.Serialize("dynamic", m_dynamicMeta);
        EnsureDynamicMetaShape(m_dynamicMeta, /*warn=*/true);
    }
}

std::vector<ScriptPointer> DPrimaryAsset::CollectExternalReferences() const
{
    std::vector<ScriptPointer> refs;

    for (DObject* obj : m_objects)
    {
        DELTA_VERIFY_MSG(obj != nullptr, "DPrimaryAsset::CollectExternalReferences encountered null object");
        VisitUnresolvedObjectReferencesInStruct(obj->GetClass(), obj,
            [&](DObjectPtrPropertyBase* /*ptrProp*/, void* /*valueAddress*/, const ScriptPointer& sp)
            {
                if (sp.IsExternal(GetAssetId()))
                    refs.push_back(sp);
            });
    }

    return refs;
}
