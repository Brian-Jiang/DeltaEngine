#include "Core/GameObject.h"

#include "Core/DWorld.h"

using namespace DeltaEngine;

GameObject::GameObject()
    : m_name("New GameObject")
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
    //for (std::shared_ptr<DComponent> component : m_components)
    //{
    //    component.reset();
    //}

    //for (std::shared_ptr<SceneComponent> sceneComponent : m_sceneComponents)
    //{
    //    sceneComponent.reset();
    //}
}

const std::string& GameObject::GetName() const { return m_name; }

DWorld* GameObject::GetCurrentWorld() const { return m_currentWorld; }

SceneComponent* GameObject::GetRootSceneComponent() const { return m_rootSceneComponent; }

const std::vector<SceneComponent*>& GameObject::GetSceneComponents() const { return m_sceneComponents; }

const std::vector<DComponent*>& GameObject::GetComponents() const { return m_components; }