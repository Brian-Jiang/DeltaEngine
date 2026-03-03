#include "Core/GameObject.h"

#include "Reflection/ReflectionRegistry.h"

#include "Core/DWorld.h"

using namespace DeltaEngine;

GameObject::GameObject()
    : m_name("New GameObject")
    , m_rootSceneComponent(nullptr)
    , m_currentWorld(nullptr)
{
	//AddComponent<Transform>();
}

//DeltaEngine::GameObject::GameObject(const std::string& name)
//    : m_name(name)
//{
//}

GameObject::~GameObject()
{
	
}

void GameObject::Destroy()
{
    for (DComponent* component : m_components)
    {
        GetReflectionRegistry().DestroyObject(component);
    }

    for (SceneComponent* sceneComponent : m_sceneComponents)
    {
        GetReflectionRegistry().DestroyObject(sceneComponent);
    }
}

const std::string& GameObject::GetName() const { return m_name; }

DWorld* GameObject::GetCurrentWorld() const { return m_currentWorld; }

SceneComponent* GameObject::GetRootSceneComponent() const { return m_rootSceneComponent; }

const std::vector<SceneComponent*>& GameObject::GetSceneComponents() const { return m_sceneComponents; }

const std::vector<DComponent*>& GameObject::GetComponents() const { return m_components; }