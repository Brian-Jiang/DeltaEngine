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

    void SelectGameObject(std::shared_ptr<GameObject> gameObject);
    void SelectComponent(std::shared_ptr<DComponent> component);
    void AddGameObjectToSelection(std::shared_ptr<GameObject> gameObject);
    void AddComponentToSelection(std::shared_ptr<DComponent> component);
    void ClearSelection();

    const std::vector<std::shared_ptr<GameObject>>& GetSelectedGameObjects() const { return m_selectedGameObjects; }
    const std::vector<std::shared_ptr<DComponent>>& GetSelectedComponents() const { return m_selectedComponents; }

    /// Returns the GameObject to use as context for ComponentsHierarchy.
    /// First selected GameObject, or the owning GameObject of the first selected component.
    std::shared_ptr<GameObject> GetContextGameObject() const;

    bool HasSelection() const;

private:
    std::vector<std::shared_ptr<GameObject>> m_selectedGameObjects;
    std::vector<std::shared_ptr<DComponent>> m_selectedComponents;
};

DELTA_ENGINE_NS_END
