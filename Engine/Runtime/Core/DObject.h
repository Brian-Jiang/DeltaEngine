#pragma once

#include "EngineIncludes.h"

//#include <type_traits>

DELTA_ENGINE_NS_BEGIN

class DClass;

class DObject
{
    friend class DClass;

public:
    DELTAENGINE_API DObject();
    DELTAENGINE_API ~DObject();

};

//template <typename T>
//concept DObjectDerived = std::is_base_of_v<DObject, T>;

DELTA_ENGINE_NS_END