#pragma once

#include "EngineIncludes.h"

#include "Runtime/Serialization/ObjectSnapshot.h"

#include <vector>

DELTA_ENGINE_NS_BEGIN

class DObject;

class DELTAENGINE_API ObjectSnapshotWriter
{
public:
    ObjectSnapshot Capture(DObject* root);

private:
    void CollectObjects(DObject* root, std::vector<DObject*>& out);
};

DELTA_ENGINE_NS_END
