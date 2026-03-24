#pragma once

#include "EditorIncludes.h"
#include "Runtime/Core/UUID.h"

#include <vector>

DELTA_ENGINE_NS_BEGIN

class EditorCore;
class GameObject;

class EditorSelectionState
{
public:
    EditorSelectionState() = default;

    // --- GameObject selection ---
    DELTAEDITOR_API void SetSelectedGameObject(ObjectId id);
    DELTAEDITOR_API void AddSelectedGameObject(ObjectId id);
    DELTAEDITOR_API void RemoveSelectedGameObject(ObjectId id);
    DELTAEDITOR_API void ClearGameObjectSelection();
    DELTAEDITOR_API const std::vector<ObjectId>& GetSelectedGameObjects() const { return m_selectedGameObjects; }
    DELTAEDITOR_API bool IsGameObjectSelected(ObjectId id) const;
    DELTAEDITOR_API bool HasGameObjectSelection() const { return !m_selectedGameObjects.empty(); }

    // --- Component selection (independent) ---
    DELTAEDITOR_API void SetSelectedComponent(ObjectId id);
    DELTAEDITOR_API void AddSelectedComponent(ObjectId id);
    DELTAEDITOR_API void RemoveSelectedComponent(ObjectId id);
    DELTAEDITOR_API void ClearComponentSelection();
    DELTAEDITOR_API const std::vector<ObjectId>& GetSelectedComponents() const { return m_selectedComponents; }
    DELTAEDITOR_API bool IsComponentSelected(ObjectId id) const;
    DELTAEDITOR_API bool HasComponentSelection() const { return !m_selectedComponents.empty(); }

    // --- Asset selection ---
    DELTAEDITOR_API void SetSelectedAsset(AssetId id);
    DELTAEDITOR_API void AddSelectedAsset(AssetId id);
    DELTAEDITOR_API void RemoveSelectedAsset(AssetId id);
    DELTAEDITOR_API void ClearAssetSelection();
    DELTAEDITOR_API const std::vector<AssetId>& GetSelectedAssets() const { return m_selectedAssets; }
    DELTAEDITOR_API bool IsAssetSelected(AssetId id) const;
    DELTAEDITOR_API bool HasAssetSelection() const { return !m_selectedAssets.empty(); }

    // --- Helpers ---
    DELTAEDITOR_API GameObject* GetContextGameObject(EditorCore& core);
    DELTAEDITOR_API void NotifyObjectDestroyed(const ObjectId& objectId);

private:
    std::vector<ObjectId> m_selectedGameObjects;
    std::vector<ObjectId> m_selectedComponents;
    std::vector<AssetId>  m_selectedAssets;
};

DELTA_ENGINE_NS_END
