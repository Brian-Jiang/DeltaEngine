#include "Core/GameObject.h"

#include "Core/DWorld.h"

using namespace DeltaEngine;

GameObject::GameObject()
{
	//AddComponent<Transform>();
}

GameObject::~GameObject()
{
	//for (std::shared_ptr<Component> component : m_components)
	//{
	//	delete component;
	//}
}

void DeltaEngine::GameObject::Destroy()
{

}
