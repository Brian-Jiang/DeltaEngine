#include "Assets/AssetDatabaseLocator.h"

#include <cassert>
#include <cstdlib>
#include <iostream>

using namespace DeltaEngine;

IAssetDatabase* AssetDatabaseLocator::s_instance = nullptr;

void AssetDatabaseLocator::Register(IAssetDatabase* db)
{
    assert(db != nullptr, "AssetDatabaseLocator::Register() called with nullptr");
    assert(s_instance == nullptr, "AssetDatabaseLocator: already registered");
    s_instance = db;
}

IAssetDatabase& AssetDatabaseLocator::Get()
{
    assert(s_instance != nullptr,
        "AssetDatabaseLocator::Get() called before Register() - call Register() before EngineMain::Initialize()");

    return *s_instance;
}
