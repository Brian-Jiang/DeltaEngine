#include "Runtime/Assets/NullAssetDatabase.h"

using namespace DeltaEngine;

DPrimaryAsset* NullAssetDatabase::LoadAsset(const AssetId& id)
{
    DLOG(LogAsset,
         ELogLevel::Verbose,
         "NullAssetDatabase::LoadAsset returning nullptr (no registered database); assetId={}",
         id.ToString());
    return nullptr;
}

bool NullAssetDatabase::IsLoaded(const AssetId& id) const
{
    DLOG(LogAsset,
         ELogLevel::Verbose,
         "NullAssetDatabase::IsLoaded returning false (no registered database); assetId={}",
         id.ToString());
    return false;
}

DObject* NullAssetDatabase::FindObject(const AssetId& assetId, const ObjectId& objId) const
{
    DLOG(LogAsset,
         ELogLevel::Verbose,
         "NullAssetDatabase::FindObject returning nullptr (no registered database); assetId={} objectId={}",
         assetId.ToString(),
         objId.ToString());
    return nullptr;
}

AssetId NullAssetDatabase::FindAssetIdByPath(const std::filesystem::path& path) const
{
    DLOG(LogAsset,
         ELogLevel::Verbose,
         "NullAssetDatabase::FindAssetIdByPath returning null id (no registered database); path={}",
         path.generic_string());
    return AssetId::Null();
}

void NullAssetDatabase::SaveAsset(const AssetId& id)
{
    DLOG(LogAsset,
         ELogLevel::Verbose,
         "NullAssetDatabase::SaveAsset ignored (no registered database); assetId={}",
         id.ToString());
}
