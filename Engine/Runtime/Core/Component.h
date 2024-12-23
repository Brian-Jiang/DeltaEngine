#pragma once

#include "EngineIncludes.h"

#include <type_traits>

#include "Core/DObject.h"

DELTA_ENGINE_NS_BEGIN

class Component: public DObject
{

public:
	Component();
	~Component();

};

template<typename T>
concept IsComponent = std::is_base_of_v<Component, T>;

DELTA_ENGINE_NS_END