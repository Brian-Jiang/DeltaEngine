#pragma once

#include "EngineIncludes.h"

#include "Core/DObject.h"
#include "Core/UUID.h"
#include "Serialization/ScriptPointer.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

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
    /// Serialized header stored alongside the asset body.
    struct Header
    {
        /// Magic value used to identify Delta asset files.
        uint32_t    m_magic       = 0x444C5441;
        /// File format version for this asset.
        uint32_t    m_fileVersion = 1;
        /// Reflected class name of the primary asset type.
        std::string m_className;
        /// Stable id used to reference this asset.
        AssetId     m_persistentId;
    };

    /// Returns the asset header.
    const Header& GetHeader() const { return m_header; }
    /// Returns the mutable asset header.
    Header& GetHeader() { return m_header; }
    /// Returns the stable asset id stored in the header.
    const AssetId& GetAssetId() const { return m_header.m_persistentId; }

    /// Adds an owned object to this asset.
    void AddObject(DObject* obj);
    /// Removes the owned object with the given id.
    void RemoveObject(const ObjectId& id);
    /// Finds an owned object by id.
    DObject* FindObject(const ObjectId& id) const;
    /// Returns all objects owned by this asset.
    const std::vector<DObject*>& GetObjects() const;

    /// Serializes the asset header.
    void SerializeHeader(AssetArchive& ar);
    /// Serializes the asset body objects.
    void SerializeBody(AssetArchive& ar);

    /// Returns true when the asset has unsaved changes.
    bool IsDirty() const  { return m_dirty; }
    /// Marks the asset as dirty.
    void MarkDirty()      { m_dirty = true; }
    /// Clears the dirty flag.
    void ClearDirty()     { m_dirty = false; }

    /// Collects unresolved references that point to other assets.
    std::vector<ScriptPointer> CollectExternalReferences() const;

    /// Collects bulk-data properties from owned objects in declaration order.
    std::vector<std::pair<DBulkDataProperty*, DObject*>> CollectBulkProperties() const;
    /// Serializes bulk payloads for owned objects.
    void SerializeBulkData(AssetArchive& ar);

private:
    void DeserializeBody(AssetArchive& ar);

    Header m_header;
    std::vector<DObject*> m_objects;
    bool m_dirty = true;
};

DELTA_ENGINE_NS_END
