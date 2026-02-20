#pragma once

#include "EngineIncludes.h"

DELTA_ENGINE_NS_BEGIN

class DClass;

class DObject
{
    friend class DClass;

public:
	DObject();
	~DObject();

};

DELTA_ENGINE_NS_END