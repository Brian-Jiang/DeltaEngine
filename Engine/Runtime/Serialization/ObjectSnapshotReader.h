#pragma once

#include "EngineIncludes.h"

DELTA_ENGINE_NS_BEGIN

class DObject;
class DPrimaryAsset;
class DScene;
class DWorld;
class IAssetDatabase;
struct ObjectSnapshot;

class DELTAENGINE_API ObjectSnapshotReader
{
public:
    DObject* Restore(const ObjectSnapshot& snapshot,
                     DWorld* world,
                     IAssetDatabase* db,
                     DPrimaryAsset* registerWithAsset = nullptr,
                     DScene* addRestoredRootGameObjectToScene = nullptr);
};

DELTA_ENGINE_NS_END
