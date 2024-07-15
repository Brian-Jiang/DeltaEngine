#pragma once

#include <type_traits>

#include "Core/Object.h"

namespace DeltaEngine
{

class Component: public Object
{
	public:
	Component();
	~Component();

};

template<typename T>
concept IsComponent = std::is_base_of_v<Component, T>;

}