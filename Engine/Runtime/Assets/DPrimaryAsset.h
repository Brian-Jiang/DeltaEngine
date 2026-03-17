#pragma once

#include "EngineIncludes.h"

#include "Core/DObject.h"
#include "Core/UUID.h"
#include "Serialization/ScriptPointer.h"

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <cstdint>

#include "DPrimaryAsset.generated.h"

DELTA_ENGINE_NS_BEGIN

class AssetArchive;
class DObjectPtrPropertyBase;
class DBulkDataProperty;

DCLASS()
class DELTAENGINE_API DPrimaryAsset : public DObject
{
    DGENERATED_BODY(DPrimaryAsset)

public:
    struct Header
    {
        uint32_t    m_magic       = 0x444C5441; // "DLTA"
        uint32_t    m_fileVersion = 1;
        std::string m_className;
        AssetId     m_persistentId;
    };

    const Header& GetHeader() const { return m_header; }
    Header& GetHeader() { return m_header; }
    const AssetId& GetAssetId() const { return m_header.m_persistentId; }

    void AddObject(std::shared_ptr<DObject> obj);
    void RemoveObject(const ObjectId& id);
    DObject* FindObject(const ObjectId& id) const;
    const std::vector<std::shared_ptr<DObject>>& GetObjects() const;

    void SerializeHeader(AssetArchive& ar);
    void SerializeBody(AssetArchive& ar);

    bool IsDirty() const  { return m_dirty; }
    void MarkDirty()      { m_dirty = true; }
    void ClearDirty()     { m_dirty = false; }

    std::vector<ScriptPointer> CollectExternalReferences() const;

    std::vector<std::pair<DBulkDataProperty*, DObject*>> CollectBulkProperties() const;
    void SerializeBulkData(AssetArchive& ar);

private:
    void DeserializeBody(AssetArchive& ar);

    Header                                m_header;
    std::vector<std::shared_ptr<DObject>> m_objects;
    bool                                  m_dirty = true;
};

DELTA_ENGINE_NS_END
