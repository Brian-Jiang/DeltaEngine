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
	
}

void GameObject::Destroy()
{
    for (std::shared_ptr<DComponent> component : m_components)
    {
        component.reset();
    }

    for (std::shared_ptr<SceneComponent> sceneComponent : m_sceneComponents)
    {
        sceneComponent.reset();
    }
}
