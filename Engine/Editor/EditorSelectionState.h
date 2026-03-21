#pragma once

#include "EditorIncludes.h"
#include "Runtime/Core/UUID.h"

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

    /// Replaces the selection with a single game object.
    DELTAEDITOR_API void SelectGameObject(GameObject* gameObject);
    /// Replaces the selection with a single component.
    DELTAEDITOR_API void SelectComponent(DComponent* component);
    /// Replaces the selection with a single asset.
    DELTAEDITOR_API void SelectAsset(const AssetId& assetId);
    /// Adds a game object to the current selection.
    DELTAEDITOR_API void AddGameObjectToSelection(GameObject* gameObject);
    /// Adds a component to the current selection.
    DELTAEDITOR_API void AddComponentToSelection(DComponent* component);
    /// Clears all selected objects, components, and assets.
    DELTAEDITOR_API void ClearSelection();

    /// Returns the selected game objects.
    DELTAEDITOR_API const std::vector<GameObject*>& GetSelectedGameObjects() const { return m_selectedGameObjects; }
    /// Returns the selected components.
    DELTAEDITOR_API const std::vector<DComponent*>& GetSelectedComponents() const { return m_selectedComponents; }
    /// Returns the selected asset id.
    DELTAEDITOR_API const AssetId& GetSelectedAssetId() const { return m_selectedAssetId; }
    /// Returns true when an asset is selected.
    DELTAEDITOR_API bool HasAssetSelection() const { return !m_selectedAssetId.IsNull(); }

    /// Returns the game object used as component-hierarchy context.
    DELTAEDITOR_API GameObject* GetContextGameObject() const;

    /// Returns true when anything is selected.
    DELTAEDITOR_API bool HasSelection() const;

private:
    std::vector<GameObject*> m_selectedGameObjects;
    std::vector<DComponent*> m_selectedComponents;
    AssetId                  m_selectedAssetId = AssetId::Null();
};

DELTA_ENGINE_NS_END
