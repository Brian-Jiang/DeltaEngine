#pragma once

#include "EngineIncludes.h"

#include "Core/UUID.h"

DELTA_ENGINE_NS_BEGIN

struct ScriptPointer
{
    /// Identifies the asset that owns the target object.
    AssetId  m_assetId  = AssetId::Null();
    /// Identifies the target object inside the owning asset.
    ObjectId m_objectId = ObjectId::Null();

    /// Returns true when both identifiers are unset.
    bool IsNull() const
    {
        return m_assetId.IsNull() && m_objectId.IsNull();
    }

    /// Returns true when the pointer targets a different asset.
    bool IsExternal(const AssetId& currentAsset) const
    {
        return !m_assetId.IsNull() && m_assetId != currentAsset;
    }

    bool operator==(const ScriptPointer&) const = default;
};

DELTA_ENGINE_NS_END
