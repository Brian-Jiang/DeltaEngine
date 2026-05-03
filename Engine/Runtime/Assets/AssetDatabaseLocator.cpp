#include "Runtime/Assets/AssetDatabaseLocator.h"

using namespace DeltaEngine;

IAssetDatabase* AssetDatabaseLocator::s_instance = nullptr;

void AssetDatabaseLocator::Register(IAssetDatabase* db)
{
    DELTA_VERIFY_MSG(db != nullptr, "AssetDatabaseLocator::Register requires non-null database pointer");
    DELTA_VERIFY_MSG(s_instance == nullptr, "AssetDatabaseLocator::Register called more than once");
    s_instance = db;
}

void AssetDatabaseLocator::Unregister()
{
    DELTA_VERIFY_MSG(s_instance != nullptr, "AssetDatabaseLocator::Unregister called without a registered instance");
    s_instance = nullptr;
}

IAssetDatabase& AssetDatabaseLocator::Get()
{
    DELTA_VERIFY_MSG(s_instance != nullptr, "AssetDatabaseLocator::Get called before Register");
    return *s_instance;
}
