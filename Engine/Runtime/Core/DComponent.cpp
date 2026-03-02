#include "Core/DComponent.h"

using namespace DeltaEngine;

DComponent::DComponent()
{
}

DComponent::~DComponent()
{
}

std::shared_ptr<GameObject> DComponent::GetGameObject() const
{
    return m_gameObject.lock();
}

const std::string& DComponent::GetName() const
{
    return m_name;
}
