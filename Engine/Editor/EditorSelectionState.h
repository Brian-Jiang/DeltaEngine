#pragma once

#include "EditorIncludes.h"
#include "Runtime/Core/Delegates/MulticastDelegate.h"
#include "Runtime/Core/UUID.h"

#include <string>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class EditorCore;
class GameObject;

class EditorSelectionState
{
public:
    EditorSelectionState() = default;

    /// Main-thread use only.
    TMulticastDelegate<void()> OnSelectionChanged;

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

    // --- Folder selection ---
    DELTAEDITOR_API void SetSelectedFolder(const std::string& relativePath);
    DELTAEDITOR_API void ClearFolderSelection();
    DELTAEDITOR_API const std::string& GetSelectedFolder() const { return m_selectedFolder; }
    DELTAEDITOR_API bool IsFolderSelected(const std::string& relativePath) const;
    DELTAEDITOR_API bool HasFolderSelection() const { return !m_selectedFolder.empty(); }

    // --- Helpers ---
    DELTAEDITOR_API void ClearAll();
    DELTAEDITOR_API GameObject* GetContextGameObject(EditorCore& core);
    DELTAEDITOR_API void NotifyObjectDestroyed(const ObjectId& objectId);

private:
    bool ClearGameObjectSelectionInternal();
    bool ClearComponentSelectionInternal();
    bool ClearAssetSelectionInternal();
    bool ClearFolderSelectionInternal();
    bool RemoveSelectedGameObjectInternal(ObjectId id);
    bool RemoveSelectedComponentInternal(ObjectId id);
    bool RemoveSelectedAssetInternal(AssetId id);
    void BroadcastSelectionChanged();

    std::vector<ObjectId> m_selectedGameObjects;
    std::vector<ObjectId> m_selectedComponents;
    std::vector<AssetId>  m_selectedAssets;
    std::string           m_selectedFolder;
};

DELTA_ENGINE_NS_END
