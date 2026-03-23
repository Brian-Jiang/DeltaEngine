#include "Assets/AssetDatabaseLocator.h"

#include <cassert>
#include <cstdlib>
#include <iostream>

using namespace DeltaEngine;

namespace
{
[[noreturn]] void FailFast(const char* message)
{
    std::cerr << "[AssetDatabase] " << message << '\n';
    assert(false && "AssetDatabaseLocator contract violation");
    std::abort();
}
}

IAssetDatabase* AssetDatabaseLocator::s_instance = nullptr;

void AssetDatabaseLocator::Register(IAssetDatabase* db)
{
    if (db == nullptr)
        FailFast("AssetDatabaseLocator::Register() called with nullptr.");

    if (s_instance != nullptr)
        FailFast("AssetDatabaseLocator::Register() called more than once.");

    s_instance = db;
}

void AssetDatabaseLocator::Unregister()
{
    if (s_instance == nullptr)
        FailFast("AssetDatabaseLocator::Unregister() called without a registered instance.");
    s_instance = nullptr;
}

IAssetDatabase& AssetDatabaseLocator::Get()
{
    if (s_instance == nullptr)
        FailFast("AssetDatabaseLocator::Get() called before Register().");

    return *s_instance;
}
