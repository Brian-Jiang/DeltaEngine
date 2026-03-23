#pragma once

#include "Assets/IAssetDatabase.h"

DELTA_ENGINE_NS_BEGIN

class DELTAENGINE_API AssetDatabaseLocator
{
public:
    /// Registers the process-wide asset database instance.
    static void Register(IAssetDatabase* db);

    /// Unregisters the process-wide asset database instance.
    static void Unregister();

    /// Returns the registered asset database instance.
    static IAssetDatabase& Get();

private:
    static IAssetDatabase* s_instance;
};

DELTA_ENGINE_NS_END
