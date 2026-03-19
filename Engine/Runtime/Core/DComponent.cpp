#include "Core/DComponent.h"

#include "Assets/DPrimaryAsset.h"

using namespace DeltaEngine;

DComponent::DComponent()
    : m_gameObject(nullptr)
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

void DComponent::RegisterComponent(GameObject* gameObject)
{
    m_gameObject = gameObject;
}

void DComponent::MarkForDestroy()
{
    m_gameObject = nullptr;
    if (HasOwningAsset())
    {
        GetOwningAsset()->RemoveObject(GetObjectId());
    }
}

void DeltaEngine::DComponent::SetName(const std::string& name)
{
    m_name = name;
}

GameObject* DComponent::GetGameObject() const { return m_gameObject; }

const std::string& DComponent::GetName() const { return m_name; }
