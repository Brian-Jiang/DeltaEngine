#pragma once

#include "EngineIncludes.h"

#include "Core/UUID.h"

DELTA_ENGINE_NS_BEGIN

struct ScriptPointer
{
    AssetId  m_assetId  = AssetId::Null();
    ObjectId m_objectId = ObjectId::Null();

    bool IsNull() const
    {
        return m_assetId.IsNull() && m_objectId.IsNull();
    }

    bool IsExternal(const AssetId& currentAsset) const
    {
        return !m_assetId.IsNull() && m_assetId != currentAsset;
    }

    bool operator==(const ScriptPointer&) const = default;
};

DELTA_ENGINE_NS_END
