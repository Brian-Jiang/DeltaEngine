#include "Core/DComponent.h"

using namespace DeltaEngine;

DComponent::DComponent()
{
}

//DComponent::DComponent(std::string name)
//    : m_name(name)
//{
//}
//
//DComponent::DComponent(std::string name, std::shared_ptr<GameObject> gameObject)
//    : m_name(name)
//    , m_gameObject(gameObject)
//{
//}

DComponent::~DComponent()
{
}

void DComponent::RegisterComponent(std::shared_ptr<GameObject> gameObject)
{
    m_gameObject = gameObject;
}

void DeltaEngine::DComponent::SetName(const std::string& name)
{
    m_name = name;
}

std::shared_ptr<GameObject> DComponent::GetGameObject() const { return m_gameObject.lock(); }

const std::string& DComponent::GetName() const { return m_name; }
