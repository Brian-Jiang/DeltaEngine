#include "Core/GameObject.h"

#include "Core/DWorld.h"

using namespace DeltaEngine;

GameObject::GameObject()
    : m_name("New GameObject")
{
	//AddComponent<Transform>();
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

const std::string& GameObject::GetName() const
{
    return m_name;
}

std::shared_ptr<DWorld> GameObject::GetCurrentWorld() const
{
    return m_currentWorld;
}

std::shared_ptr<SceneComponent> GameObject::GetRootSceneComponent() const
{
    return m_rootSceneComponent.lock();
}

const std::vector<std::shared_ptr<SceneComponent>>& GameObject::GetSceneComponents() const
{
    return m_sceneComponents;
}

const std::vector<std::shared_ptr<DComponent>>& GameObject::GetComponents() const
{
    return m_components;
}
