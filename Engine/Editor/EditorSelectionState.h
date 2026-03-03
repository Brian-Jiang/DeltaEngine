#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class GameObject;
class DComponent;

/// Records which GameObjects and components are selected in the editor.
/// Used by WorldOutliner, ComponentsHierarchy, and Details windows.
class EditorSelectionState
{
public:
    EditorSelectionState() = default;

    void SelectGameObject(GameObject* gameObject);
    void SelectComponent(DComponent* component);
    void AddGameObjectToSelection(GameObject* gameObject);
    void AddComponentToSelection(DComponent* component);
    void ClearSelection();

    const std::vector<GameObject*>& GetSelectedGameObjects() const { return m_selectedGameObjects; }
    const std::vector<DComponent*>& GetSelectedComponents() const { return m_selectedComponents; }

    /// Returns the GameObject to use as context for ComponentsHierarchy.
    /// First selected GameObject, or the owning GameObject of the first selected component.
    GameObject* GetContextGameObject() const;

    bool HasSelection() const;

private:
    std::vector<GameObject*> m_selectedGameObjects;
    std::vector<DComponent*> m_selectedComponents;
};

DELTA_ENGINE_NS_END
