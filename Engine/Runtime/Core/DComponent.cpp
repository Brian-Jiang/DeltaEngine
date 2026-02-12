#include "Core/DComponent.h"

using namespace DeltaEngine;

DComponent::DComponent()
{
}

DeltaEngine::DComponent::DComponent(std::shared_ptr<GameObject> gameObject)
    : m_gameObject(gameObject)
{
}

DComponent::~DComponent()
{
}
