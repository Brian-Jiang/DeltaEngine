#include "Core/GameObject.h"

#include "Core/Transform.h"

using namespace DeltaEngine;

GameObject::GameObject(): components()
{
	AddComponent<Transform>();
}

GameObject::~GameObject()
{
	for (Component *component : components)
	{
		delete component;
	}
}

template <typename T>
requires IsComponent<T>
T *GameObject::AddComponent()
{
	T *component = new T();
	components.push_back(component);
	return component;
}
