#pragma once

#include "EngineIncludes.h"

#include "DObject.generated.h"

DELTA_ENGINE_NS_BEGIN

class DClass;

DCLASS()
class DObject
{
    DGENERATED_BODY(DObject)
    friend class DClass;

public:
    DELTAENGINE_API DObject();
    DELTAENGINE_API virtual ~DObject();

};

//template <typename T>
//concept DObjectDerived = std::is_base_of_v<DObject, T>;

DELTA_ENGINE_NS_END