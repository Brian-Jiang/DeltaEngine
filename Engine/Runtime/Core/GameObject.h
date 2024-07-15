#pragma once

#include <vector>

#include "Core/Object.h"
#include "Core/Component.h"

namespace DeltaEngine
{

class GameObject: public Object
{
public:
	GameObject();
	~GameObject();

	template <typename T>
	requires IsComponent<T>
	T *AddComponent();

private:
	std::vector<Component*> components;
};

}
