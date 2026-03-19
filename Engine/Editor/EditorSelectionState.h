#pragma once

#include "EngineIncludes.h"
#include "Runtime/Core/UUID.h"

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
    void SelectAsset(const AssetId& assetId);
    void AddGameObjectToSelection(GameObject* gameObject);
    void AddComponentToSelection(DComponent* component);
    void ClearSelection();

    const std::vector<GameObject*>& GetSelectedGameObjects() const { return m_selectedGameObjects; }
    const std::vector<DComponent*>& GetSelectedComponents() const { return m_selectedComponents; }
    const AssetId& GetSelectedAssetId() const { return m_selectedAssetId; }
    bool HasAssetSelection() const { return !m_selectedAssetId.IsNull(); }

    /// Returns the GameObject to use as context for ComponentsHierarchy.
    /// First selected GameObject, or the owning GameObject of the first selected component.
    GameObject* GetContextGameObject() const;

    bool HasSelection() const;

private:
    std::vector<GameObject*> m_selectedGameObjects;
    std::vector<DComponent*> m_selectedComponents;
    AssetId                  m_selectedAssetId = AssetId::Null();
};

DELTA_ENGINE_NS_END
