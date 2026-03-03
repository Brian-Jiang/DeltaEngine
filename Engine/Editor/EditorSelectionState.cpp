#include "Editor/EditorSelectionState.h"

#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/DComponent.h"

using namespace DeltaEngine;

void EditorSelectionState::SelectGameObject(GameObject* gameObject)
{
    m_selectedGameObjects.clear();
    m_selectedComponents.clear();
    if (gameObject)
    {
        m_selectedGameObjects.push_back(gameObject);
    }
}

void EditorSelectionState::SelectComponent(DComponent* component)
{
    m_selectedComponents.clear();
    m_selectedGameObjects.clear();
    if (component)
    {
        m_selectedComponents.push_back(component);
    }
}

void EditorSelectionState::AddGameObjectToSelection(GameObject* gameObject)
{
    if (gameObject)
    {
        m_selectedGameObjects.push_back(gameObject);
    }
}

void EditorSelectionState::AddComponentToSelection(DComponent* component)
{
    if (component)
    {
        m_selectedComponents.push_back(component);
    }
}

void EditorSelectionState::ClearSelection()
{
    m_selectedGameObjects.clear();
    m_selectedComponents.clear();
}

GameObject* EditorSelectionState::GetContextGameObject() const
{
    if (!m_selectedGameObjects.empty())
    {
        return m_selectedGameObjects.front();
    }
    if (!m_selectedComponents.empty())
    {
        return m_selectedComponents.front()->GetGameObject();
    }
    return nullptr;
}

bool EditorSelectionState::HasSelection() const
{
    return !m_selectedGameObjects.empty() || !m_selectedComponents.empty();
}
