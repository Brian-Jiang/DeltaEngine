#pragma once

#include "Assets/IAssetDatabase.h"

DELTA_ENGINE_NS_BEGIN

class DELTAENGINE_API AssetDatabaseLocator
{
public:
    // Must be called before EngineMain::Initialize().
    static void Register(IAssetDatabase* db);

    // Fails fast if no database has been registered.
    static IAssetDatabase& Get();

private:
    static IAssetDatabase* s_instance;
};

DELTA_ENGINE_NS_END
