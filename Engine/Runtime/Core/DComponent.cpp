#include "Core/DComponent.h"

using namespace DeltaEngine;

DComponent::DComponent()
{
}

DComponent::DComponent(std::string name)
    : m_name(name)
{
}

DComponent::DComponent(std::string name, std::shared_ptr<GameObject> gameObject)
    : m_name(name)
    , m_gameObject(gameObject)
{
}

DComponent::~DComponent()
{
}
