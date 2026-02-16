#include "Core/GameObject.h"

#include "Core/DWorld.h"

using namespace DeltaEngine;

GameObject::GameObject()
    : m_name("New GameObject")
{
	//AddComponent<Transform>();
}

DeltaEngine::GameObject::GameObject(const std::string& name)
    : m_name(name)
{
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
